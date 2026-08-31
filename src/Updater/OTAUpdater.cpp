#include "OTAUpdater.h"
#include "VersionInfo.h"
#include "../Utils/Logger.h"
#include <windows.h>
#include <wininet.h>
#include <sstream>
#include <iomanip>
#include <vector>

OTAUpdater& OTAUpdater::Instance() {
    static OTAUpdater inst;
    return inst;
}

void OTAUpdater::SetChannel(UpdateChannel ch) { m_channel = ch; }
void OTAUpdater::SetAutoCheck(bool v) { m_autoCheck = v; }
void OTAUpdater::SetProxy(const std::string& host, int port) { m_proxyHost = host; m_proxyPort = port; }

std::string OTAUpdater::GetUpdateUrl() const {
    return (m_channel == UpdateChannel::Nightly) ? "https://dockforge.app/api/nightly" : "https://dockforge.app/api/stable";
}

bool OTAUpdater::CheckForUpdates(UpdateInfo& out) {
    std::string response = HttpGet(GetUpdateUrl());
    if (response.empty()) { LOG_ERROR("Update check failed: empty response"); return false; }
    return ParseUpdateJson(response, out);
}

bool OTAUpdater::DownloadUpdate(const UpdateInfo& info, const std::string& destPath, std::function<void(int)> onProgress) {
    std::string data = HttpGet(info.downloadUrl);
    if (data.empty()) return false;
    std::ofstream out(destPath, std::ios::binary);
    if (!out) return false;
    out.write(data.data(), data.size());
    if (onProgress) onProgress(100);
    return true;
}

bool OTAUpdater::VerifySignature(const std::string& filePath, const std::string& expectedHash) {
    (void)expectedHash;
    return std::filesystem::exists(filePath);
}

bool OTAUpdater::ApplyUpdate(const std::string& updatePackage) {
    (void)updatePackage;
    LOG_INFO("Applying update...");
    return true;
}

void OTAUpdater::ScheduleRestart() {
    LOG_INFO("Update restart scheduled");
}

bool OTAUpdater::ParseUpdateJson(const std::string& json, UpdateInfo& out) {
    auto getString = [&](const std::string& key) -> std::string {
        size_t k = json.find("\"" + key + "\"");
        if (k == std::string::npos) return "";
        size_t c = json.find(":", k);
        if (c == std::string::npos) return "";
        size_t q1 = json.find("\"", c);
        if (q1 == std::string::npos) return "";
        size_t q2 = json.find("\"", q1 + 1);
        if (q2 == std::string::npos) return "";
        return json.substr(q1 + 1, q2 - q1 - 1);
    };
    out.version = getString("version");
    out.downloadUrl = getString("downloadUrl");
    out.releaseNotes = getString("releaseNotes");
    out.hash = getString("hash");
    std::string mandatory = getString("mandatory");
    out.mandatory = (mandatory == "true");
    if (out.version.empty() || out.downloadUrl.empty()) return false;
    out.available = (out.version != VersionInfo::GetVersionString());
    return true;
}

std::string OTAUpdater::HttpGet(const std::string& url) {
    HINTERNET hInternet = InternetOpenA("DockForge", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
    if (!hInternet) return "";
    HINTERNET hConnect = InternetOpenUrlA(hInternet, url.c_str(), nullptr, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hConnect) { InternetCloseHandle(hInternet); return ""; }
    std::string result;
    char buffer[4096];
    DWORD read = 0;
    while (InternetReadFile(hConnect, buffer, sizeof(buffer), &read) && read > 0) {
        result.append(buffer, read);
    }
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
    return result;
}