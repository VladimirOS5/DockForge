#include "OTAUpdater.h"
#include "../Utils/Logger.h"
#include <windows.h>
#include <wininet.h>
#include <fstream>
#include <sstream>

#pragma comment(lib, "wininet.lib")

OTAUpdater& OTAUpdater::Instance() {
    static OTAUpdater instance;
    return instance;
}

OTAUpdater::~OTAUpdater() {
    m_cancelFlag = true;
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
}

void OTAUpdater::SetChannel(const std::string& channel) {
    m_channel = channel;
}

std::string OTAUpdater::GetUpdateUrl() const {
    return BuildUpdateUrl();
}

bool OTAUpdater::HttpGet(const std::string& url, std::string& response) {
    HINTERNET hInternet = InternetOpenA("DockForge", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
    if (!hInternet) return false;

    HINTERNET hConnect = InternetOpenUrlA(hInternet, url.c_str(), nullptr, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hConnect) {
        InternetCloseHandle(hInternet);
        return false;
    }

    char buffer[4096];
    DWORD bytesRead;
    response.clear();
    while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        response.append(buffer, bytesRead);
    }

    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
    return !response.empty();
}

bool OTAUpdater::ParseUpdateJson(const std::string& json, ReleaseInfo& out) {
    // Simple JSON parsing - in production use a proper library
    auto findValue = [&](const std::string& key) -> std::string {
        size_t pos = json.find("\"" + key + "\"");
        if (pos == std::string::npos) return "";
        pos = json.find(":", pos);
        if (pos == std::string::npos) return "";
        pos++;
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == '"')) pos++;
        size_t end = json.find_first_of("\",", pos);
        return json.substr(pos, end - pos);
    };

    std::string versionStr = findValue("version");
    if (!versionStr.empty()) {
        out.version = SemanticVersion::FromString(versionStr);
    }
    out.downloadUrl = findValue("downloadUrl");
    out.checksum = findValue("checksum");
    return true;
}

void OTAUpdater::CheckForUpdatesAsync() {
    if (m_operationInProgress) return;
    m_operationInProgress = true;
    m_workerThread = std::thread([this]() {
        CheckWorker();
    });
}

void OTAUpdater::CheckWorker() {
    std::string url = BuildUpdateUrl();
    std::string response;
    if (!HttpGet(url, response)) {
        SetState(UpdateState::Error, UpdateError::NetworkError, "Failed to fetch update info");
        m_operationInProgress = false;
        if (m_completionCallback) m_completionCallback(false, "Network error");
        return;
    }

    ReleaseInfo release;
    if (!ParseUpdateJson(response, release)) {
        SetState(UpdateState::Error, UpdateError::ParseError, "Failed to parse update info");
        m_operationInProgress = false;
        if (m_completionCallback) m_completionCallback(false, "Parse error");
        return;
    }

    SemanticVersion current = SemanticVersion::Current();
    if (release.version.IsNewerThan(current)) {
        m_pendingRelease = release;
        SetState(UpdateState::UpdateAvailable);
        if (m_autoDownload) {
            DownloadUpdateAsync();
        } else {
            m_operationInProgress = false;
            if (m_completionCallback) m_completionCallback(true, "Update available: " + release.version.ToString());
        }
    } else {
        SetState(UpdateState::Idle);
        m_operationInProgress = false;
        if (m_completionCallback) m_completionCallback(false, "No update available");
    }
}

void OTAUpdater::DownloadUpdateAsync() {
    if (m_pendingRelease.downloadUrl.empty()) return;
    m_operationInProgress = true;
    m_workerThread = std::thread([this]() {
        DownloadWorker();
    });
}

void OTAUpdater::DownloadWorker() {
    std::filesystem::path tempPath = std::filesystem::temp_directory_path() / "dockforge_update.exe";
    float progress = 0.0f;
    
    if (!DownloadFile(m_pendingRelease.downloadUrl, tempPath, m_cancelFlag, progress)) {
        SetState(UpdateState::Error, UpdateError::DownloadFailed, "Download failed");
        m_operationInProgress = false;
        if (m_completionCallback) m_completionCallback(false, "Download failed");
        return;
    }

    m_installerPath = tempPath;
    SetState(UpdateState::Downloaded);
    m_operationInProgress = false;
    
    if (m_autoInstall) {
        InstallUpdate();
    } else if (m_completionCallback) {
        m_completionCallback(true, "Update downloaded and ready to install");
    }
}

bool OTAUpdater::DownloadFile(const std::string& url, const std::filesystem::path& dest, 
                               std::atomic<bool>& cancel, float& progress) {
    HINTERNET hInternet = InternetOpenA("DockForge", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
    if (!hInternet) return false;

    HINTERNET hUrl = InternetOpenUrlA(hInternet, url.c_str(), nullptr, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hUrl) {
        InternetCloseHandle(hInternet);
        return false;
    }

    std::ofstream out(dest, std::ios::binary);
    char buffer[8192];
    DWORD bytesRead;
    size_t totalBytes = 0;
    
    while (!cancel && InternetReadFile(hUrl, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        out.write(buffer, bytesRead);
        totalBytes += bytesRead;
        progress = static_cast<float>(totalBytes) / 1000000.0f; // Rough progress estimate
        NotifyProgress();
    }

    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInternet);
    return !cancel;
}

void OTAUpdater::InstallUpdate() {
    if (m_installerPath.empty()) return;
    
    SHELLEXECUTEINFOA sei = { sizeof(sei) };
    sei.lpVerb = "open";
    sei.lpFile = m_installerPath.string().c_str();
    sei.nShow = SW_SHOW;
    
    if (ShellExecuteExA(&sei)) {
        SetState(UpdateState::Installing);
    } else {
        SetState(UpdateState::Error, UpdateError::InstallFailed, "Failed to start installer");
    }
}

void OTAUpdater::RollbackUpdate() {
    if (!m_backupPath.empty() && std::filesystem::exists(m_backupPath)) {
        // Restore from backup
        LOG_INFO("Rolling back update...");
    }
}

void OTAUpdater::CancelOperation() {
    m_cancelFlag = true;
}

UpdateProgress OTAUpdater::GetProgress() const {
    return m_progress;
}

bool OTAUpdater::IsOperationInProgress() const {
    return m_operationInProgress;
}

bool OTAUpdater::IsUpdatePending() const {
    return m_progress.state == UpdateState::UpdateAvailable || 
           m_progress.state == UpdateState::Downloaded;
}

std::filesystem::path OTAUpdater::GetPendingInstallerPath() const {
    return m_installerPath;
}

void OTAUpdater::Update(float deltaTime) {
    if (!m_autoCheck) return;
    
    m_checkTimer += deltaTime;
    if (m_checkTimer >= m_checkIntervalMinutes * 60.0f) {
        m_checkTimer = 0.0f;
        CheckForUpdatesAsync();
    }
}

void OTAUpdater::CleanupOldInstallers() {
    try {
        auto tempPath = std::filesystem::temp_directory_path();
        for (const auto& entry : std::filesystem::directory_iterator(tempPath)) {
            if (entry.path().filename().string().find("dockforge_update") != std::string::npos) {
                std::filesystem::remove(entry.path());
            }
        }
    } catch (...) {}
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

std::string OTAUpdater::BuildUpdateUrl() const {
    return m_updateUrl + "/check?channel=" + m_channel + "&version=" + SemanticVersion::Current().ToString();
}

bool OTAUpdater::VerifyChecksum(const std::filesystem::path& file, const std::string& expectedSha256) {
    // TODO: Implement SHA-256 verification
    (void)file;
    (void)expectedSha256;
    return true;
}

bool OTAUpdater::IsWindowsVersionSupported(const std::string& minVersion) {
    // TODO: Implement Windows version check
    (void)minVersion;
    return true;
}
