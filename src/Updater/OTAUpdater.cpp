#include "OTAUpdater.h"
#include "../Utils/Logger.h"
#include "../Utils/Config.h"
#include <windows.h>
#include <wininet.h>
#include <wincrypt.h>
#include <fstream>
#include <sstream>
#include <shlobj.h>
#include <json.hpp>

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "crypt32.lib")

using json = nlohmann::json;

OTAUpdater& OTAUpdater::Instance() {
    static OTAUpdater instance;
    return instance;
}

OTAUpdater::~OTAUpdater() {
    CancelOperation();
    if (m_workerThread.joinable()) m_workerThread.join();
}

void OTAUpdater::SetState(UpdateState state, UpdateError error, const std::string& details) {
    m_progress.state = state;
    m_progress.error = error;
    m_progress.errorDetails = details;
    NotifyProgress();
}

void OTAUpdater::NotifyProgress() {
    if (m_progressCallback) {
        m_progressCallback(m_progress);
    }
}

UpdateProgress OTAUpdater::GetProgress() const {
    return m_progress;
}

bool OTAUpdater::IsOperationInProgress() const {
    return m_operationInProgress;
}

bool OTAUpdater::IsUpdatePending() const {
    return m_progress.state == UpdateState::Downloaded || 
           m_progress.state == UpdateState::Verified;
}

std::filesystem::path OTAUpdater::GetPendingInstallerPath() const {
    return m_installerPath;
}

void OTAUpdater::CancelOperation() {
    m_cancelFlag = true;
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
    m_cancelFlag = false;
    m_operationInProgress = false;
}

std::string OTAUpdater::BuildUpdateUrl() const {
    std::string url = m_updateUrl;
    url += "?channel=" + m_channel;
    url += "&version=" + SemanticVersion::Current().ToString();
    url += "&arch=" + std::string(sizeof(void*) == 8 ? "x64" : "x86");
    return url;
}

bool OTAUpdater::ParseReleaseJson(const std::string& jsonStr, ReleaseInfo& out) {
    try {
        json j = json::parse(jsonStr);
        if (!j.contains("version") || !j.contains("download_url")) return false;

        out.version = SemanticVersion::FromString(j.value("version", "0.0.0"));
        out.downloadUrl = j.value("download_url", "");
        out.changelog = j.value("changelog", "");
        out.checksum = j.value("checksum_sha256", "");
        out.fileSize = j.value("file_size", 0);
        out.mandatory = j.value("mandatory", false);
        out.minWindowsVersion = j.value("min_windows_version", "");
        return out.version.IsValid() && !out.downloadUrl.empty();
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("Failed to parse release JSON: ") + e.what());
        return false;
    }
}

bool OTAUpdater::IsWindowsVersionSupported(const std::string& minVersion) {
    if (minVersion.empty()) return true;

    OSVERSIONINFOEXW osvi = { sizeof(osvi) };
    DWORDLONG condMask = 0;
    VER_SET_CONDITION(condMask, VER_MAJORVERSION, VER_GREATER_EQUAL);
    VER_SET_CONDITION(condMask, VER_MINORVERSION, VER_GREATER_EQUAL);
    VER_SET_CONDITION(condMask, VER_BUILDNUMBER, VER_GREATER_EQUAL);

    // Parse min version
    int maj = 0, min = 0, build = 0;
    sscanf_s(minVersion.c_str(), "%d.%d.%d", &maj, &min, &build);

    osvi.dwMajorVersion = maj;
    osvi.dwMinorVersion = min;
    osvi.dwBuildNumber = build;

    return VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION | VER_BUILDNUMBER, condMask) != FALSE;
}

bool OTAUpdater::DownloadFile(const std::string& url, const std::filesystem::path& dest, 
                               std::atomic<bool>& cancel, float& progress) {
    HINTERNET hInternet = InternetOpenA("DockForge Updater/1.0", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
    if (!hInternet) return false;

    HINTERNET hUrl = InternetOpenUrlA(hInternet, url.c_str(), nullptr, 0, 
        INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE, 0);
    if (!hUrl) {
        InternetCloseHandle(hInternet);
        return false;
    }

    DWORD contentLength = 0;
    DWORD lengthSize = sizeof(contentLength);
    DWORD index = 0;
    HttpQueryInfoA(hUrl, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &contentLength, &lengthSize, &index);

    std::ofstream file(dest, std::ios::binary);
    if (!file) {
        InternetCloseHandle(hUrl);
        InternetCloseHandle(hInternet);
        return false;
    }

    char buffer[8192];
    DWORD read = 0;
    DWORD totalRead = 0;
    while (InternetReadFile(hUrl, buffer, sizeof(buffer), &read) && read > 0) {
        if (cancel) break;
        file.write(buffer, read);
        totalRead += read;
        if (contentLength > 0) {
            progress = static_cast<float>(totalRead) / static_cast<float>(contentLength) * 100.0f;
        }
    }

    file.close();
    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInternet);

    return !cancel && totalRead > 0;
}

bool OTAUpdater::VerifyChecksum(const std::filesystem::path& file, const std::string& expectedSha256) {
    if (expectedSha256.empty()) {
        LOG_WARN("No checksum provided, skipping verification");
        return true;
    }

    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    bool result = false;

    if (!CryptAcquireContext(&hProv, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) return false;
    if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
        CryptReleaseContext(hProv, 0);
        return false;
    }

    std::ifstream f(file, std::ios::binary);
    if (!f) goto cleanup;

    char buf[4096];
    while (f.read(buf, sizeof(buf)) || f.gcount() > 0) {
        CryptHashData(hHash, reinterpret_cast<BYTE*>(buf), static_cast<DWORD>(f.gcount()), 0);
    }

    BYTE hash[32];
    DWORD hashLen = 32;
    if (CryptGetHashParam(hHash, HP_HASHVAL, hash, &hashLen, 0)) {
        std::stringstream ss;
        for (int i = 0; i < 32; ++i) ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
        std::string actual = ss.str();
        // Case-insensitive compare
        std::string expectedLower = expectedSha256;
        std::transform(expectedLower.begin(), expectedLower.end(), expectedLower.begin(), ::tolower);
        result = (actual == expectedLower);
        if (!result) {
            LOG_ERROR("Checksum mismatch! Expected: " + expectedLower + ", Got: " + actual);
        }
    }

cleanup:
    if (hHash) CryptDestroyHash(hHash);
    if (hProv) CryptReleaseContext(hProv, 0);
    return result;
}

void OTAUpdater::CheckForUpdateAsync() {
    if (m_operationInProgress) return;
    m_operationInProgress = true;
    m_cancelFlag = false;
    if (m_workerThread.joinable()) m_workerThread.join();
    m_workerThread = std::thread(&OTAUpdater::CheckWorker, this);
}

void OTAUpdater::CheckWorker() {
    SetState(UpdateState::Checking);

    HINTERNET hInternet = InternetOpenA("DockForge Updater/1.0", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
    if (!hInternet) {
        SetState(UpdateState::Error, UpdateError::NetworkError, "Failed to open internet connection");
        m_operationInProgress = false;
        return;
    }

    std::string url = BuildUpdateUrl();
    HINTERNET hUrl = InternetOpenUrlA(hInternet, url.c_str(), nullptr, 0,
        INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE, 0);
    if (!hUrl) {
        InternetCloseHandle(hInternet);
        SetState(UpdateState::Error, UpdateError::NetworkError, "Failed to open update URL");
        m_operationInProgress = false;
        return;
    }

    std::string response;
    char buffer[4096];
    DWORD read = 0;
    while (InternetReadFile(hUrl, buffer, sizeof(buffer) - 1, &read) && read > 0) {
        buffer[read] = '\0';
        response += buffer;
    }

    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInternet);

    if (m_cancelFlag) {
        SetState(UpdateState::Idle);
        m_operationInProgress = false;
        return;
    }

    ReleaseInfo info;
    if (!ParseReleaseJson(response, info)) {
        SetState(UpdateState::Error, UpdateError::ParseError, "Failed to parse server response");
        m_operationInProgress = false;
        return;
    }

    if (!info.version.IsNewerThan(SemanticVersion::Current())) {
        SetState(UpdateState::Idle, UpdateError::None, "No update available");
        m_operationInProgress = false;
        if (m_completionCallback) m_completionCallback(true, "You are on the latest version");
        return;
    }

    if (!IsWindowsVersionSupported(info.minWindowsVersion)) {
        SetState(UpdateState::Error, UpdateError::WindowsVersionUnsupported, 
            "Requires Windows " + info.minWindowsVersion);
        m_operationInProgress = false;
        return;
    }

    m_pendingRelease = info;
    m_progress.targetVersion = info.version;
    SetState(UpdateState::UpdateAvailable, UpdateError::None, 
        "Version " + info.version.ToString() + " is available");
    m_operationInProgress = false;

    if (m_completionCallback) {
        m_completionCallback(true, "Update available: " + info.version.ToString());
    }

    // Auto-download if enabled
    if (m_autoDownload) {
        DownloadUpdateAsync();
    }
}

void OTAUpdater::DownloadUpdateAsync() {
    if (m_operationInProgress) return;
    if (m_pendingRelease.downloadUrl.empty()) {
        SetState(UpdateState::Error, UpdateError::DownloadFailed, "No download URL available");
        return;
    }
    m_operationInProgress = true;
    m_cancelFlag = false;
    if (m_workerThread.joinable()) m_workerThread.join();
    m_workerThread = std::thread(&OTAUpdater::DownloadWorker, this);
}

void OTAUpdater::DownloadWorker() {
    SetState(UpdateState::Downloading);

    wchar_t localAppData[MAX_PATH];
    if (FAILED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, localAppData))) {
        SetState(UpdateState::Error, UpdateError::NoWritePermission, "Cannot access AppData");
        m_operationInProgress = false;
        return;
    }

    std::filesystem::path updateDir = std::filesystem::path(localAppData) / L"DockForge" / L"updates";
    try {
        std::filesystem::create_directories(updateDir);
    } catch (...) {
        SetState(UpdateState::Error, UpdateError::NoWritePermission, "Cannot create update directory");
        m_operationInProgress = false;
        return;
    }

    std::string filename = "DockForge_" + m_pendingRelease.version.ToString() + "_Setup.exe";
    std::filesystem::path destPath = updateDir / filename;
    m_installerPath = destPath;

    // Remove old incomplete download
    if (std::filesystem::exists(destPath)) {
        std::filesystem::remove(destPath);
    }

    float progress = 0.0f;
    bool success = DownloadFile(m_pendingRelease.downloadUrl, destPath, m_cancelFlag, progress);

    if (m_cancelFlag) {
        SetState(UpdateState::Idle);
        m_operationInProgress = false;
        return;
    }

    if (!success) {
        SetState(UpdateState::Error, UpdateError::DownloadFailed, "Download failed or was interrupted");
        m_operationInProgress = false;
        return;
    }

    // Verify checksum
    SetState(UpdateState::Verifying);
    if (!VerifyChecksum(destPath, m_pendingRelease.checksum)) {
        std::filesystem::remove(destPath);
        SetState(UpdateState::Error, UpdateError::ChecksumMismatch, "Downloaded file is corrupted");
        m_operationInProgress = false;
        return;
    }

    SetState(UpdateState::Verified, UpdateError::None, "Update ready to install");
    m_operationInProgress = false;

    if (m_completionCallback) {
        m_completionCallback(true, "Update downloaded and verified");
    }

    // Auto-install if enabled
    if (m_autoInstall) {
        InstallUpdate();
    }
}

void OTAUpdater::InstallUpdate() {
    if (!IsUpdatePending()) {
        LOG_WARN("No update pending installation");
        return;
    }

    SetState(UpdateState::Installing);

    if (!std::filesystem::exists(m_installerPath)) {
        SetState(UpdateState::Error, UpdateError::InstallFailed, "Installer file not found");
        return;
    }

    // Create backup of current executable for rollback
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    m_backupPath = std::filesystem::path(exePath);
    m_backupPath += L".backup";
    try {
        std::filesystem::copy_file(exePath, m_backupPath, std::filesystem::copy_options::overwrite_existing);
        LOG_INFO("Backup created: " + m_backupPath.string());
    } catch (...) {
        LOG_WARN("Failed to create backup, continuing without rollback protection");
    }

    // Launch installer with /SILENT and /CLOSEAPPLICATIONS flags
    std::wstring cmdLine = L"\"" + m_installerPath.wstring() + L"\" /SILENT /CLOSEAPPLICATIONS /NORESTART";

    SHELLEXECUTEINFOW sei = { sizeof(sei) };
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb = L"open";
    sei.lpFile = m_installerPath.c_str();
    sei.lpParameters = L"/SILENT /CLOSEAPPLICATIONS /NORESTART";
    sei.nShow = SW_HIDE;

    if (!ShellExecuteExW(&sei)) {
        SetState(UpdateState::Error, UpdateError::InstallFailed, "Failed to launch installer");
        return;
    }

    SetState(UpdateState::Installed, UpdateError::None, "Installer launched. DockForge will close.");

    // Signal application to quit gracefully
    if (m_completionCallback) {
        m_completionCallback(true, "Installing update...");
    }

    // Schedule self-termination after short delay to let installer take over
    // In real implementation, this would be handled by Application::RequestQuit()
    LOG_INFO("Update installation initiated. Exiting...");
}

void OTAUpdater::RollbackUpdate() {
    if (!std::filesystem::exists(m_backupPath)) {
        LOG_ERROR("No backup available for rollback");
        SetState(UpdateState::Error, UpdateError::InstallFailed, "No backup found");
        return;
    }

    try {
        wchar_t exePath[MAX_PATH];
        GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        std::filesystem::copy_file(m_backupPath, exePath, std::filesystem::copy_options::overwrite_existing);
        LOG_INFO("Rollback complete. Previous version restored.");
        SetState(UpdateState::RolledBack);
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("Rollback failed: ") + e.what());
        SetState(UpdateState::Error, UpdateError::InstallFailed, "Rollback failed");
    }
}

void OTAUpdater::Update(float deltaTime) {
    if (m_checkIntervalMinutes <= 0) return;
    m_checkTimer += deltaTime;
    if (m_checkTimer >= m_checkIntervalMinutes * 60.0f) {
        m_checkTimer = 0.0f;
        if (!IsOperationInProgress() && m_progress.state != UpdateState::UpdateAvailable &&
            m_progress.state != UpdateState::Downloaded && m_progress.state != UpdateState::Verified) {
            CheckForUpdateAsync();
        }
    }
}

void OTAUpdater::CleanupOldInstallers() {
    wchar_t localAppData[MAX_PATH];
    if (FAILED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, localAppData))) return;

    std::filesystem::path updateDir = std::filesystem::path(localAppData) / L"DockForge" / L"updates";
    if (!std::filesystem::exists(updateDir)) return;

    try {
        auto now = std::filesystem::file_time_type::clock::now();
        for (const auto& entry : std::filesystem::directory_iterator(updateDir)) {
            if (!entry.is_regular_file()) continue;
            auto age = now - entry.last_write_time();
            auto hours = std::chrono::duration_cast<std::chrono::hours>(age).count();
            if (hours > 168) { // Delete installers older than 7 days
                std::filesystem::remove(entry.path());
                LOG_INFO("Cleaned up old installer: " + entry.path().string());
            }
        }
    } catch (...) {
        LOG_WARN("Failed to cleanup old installers");
    }
}
