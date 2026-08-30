#include "OTAUpdater.h"
#include "../Utils/Logger.h"
#include <windows.h>
#include <wininet.h>
#include <fstream>
#include <sstream>
#include <json/json.h>

void OTAUpdater::CheckForUpdate() {
    if (m_operationInProgress) return;
    m_operationInProgress = true;
    auto result = CheckForUpdateInternal();
    m_operationInProgress = false;
    if (m_completionCallback) {
        m_completionCallback(result.first, result.second);
    }
}

std::pair<bool, std::string> OTAUpdater::CheckForUpdateInternal() {
    HINTERNET hInternet = InternetOpenA("DockForge", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
    if (!hInternet) return {false, "Failed to open internet"};

    std::string url = m_updateUrl + "/check?channel=" + m_channel + "&version=" + VersionInfo::GetVersionString();
    HINTERNET hConnect = InternetOpenUrlA(hInternet, url.c_str(), nullptr, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hConnect) {
        InternetCloseHandle(hInternet);
        return {false, "Failed to connect"};
    }

    char buffer[4096];
    DWORD bytesRead;
    std::string response;
    while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        response.append(buffer, bytesRead);
    }

    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);

    if (response.empty()) return {false, "Empty response"};

    try {
        Json::Value root;
        Json::Reader reader;
        if (!reader.parse(response, root)) return {false, "JSON parse error"};

        bool available = root.get("available", false).asBool();
        if (!available) return {false, "No update available"};

        m_latestVersion = ParseVersionJson(root["version"].toStyledString());
        m_updateUrl = root["url"].asString();
        m_updatePending = true;
        return {true, "Update available: " + m_latestVersion.GetVersionString()};
    } catch (...) {
        return {false, "Exception during check"};
    }
}

bool OTAUpdater::DownloadUpdate(const std::string& url, const std::string& path) {
    HINTERNET hInternet = InternetOpenA("DockForge", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
    if (!hInternet) return false;

    HINTERNET hUrl = InternetOpenUrlA(hInternet, url.c_str(), nullptr, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hUrl) {
        InternetCloseHandle(hInternet);
        return false;
    }

    std::ofstream out(path, std::ios::binary);
    char buffer[8192];
    DWORD bytesRead;
    while (InternetReadFile(hUrl, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        out.write(buffer, bytesRead);
        if (m_progressCallback) {
            UpdateProgress p;
            p.percent = 50.0f;
            m_progressCallback(p);
        }
    }

    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInternet);
    return true;
}

bool OTAUpdater::ApplyUpdate(const std::string& path) {
    SHELLEXECUTEINFOA sei = { sizeof(sei) };
    sei.lpVerb = "open";
    sei.lpFile = path.c_str();
    sei.nShow = SW_SHOW;
    return ShellExecuteExA(&sei) == TRUE;
}

VersionInfo OTAUpdater::ParseVersionJson(const std::string& json) {
    Json::Value root;
    Json::Reader reader;
    if (reader.parse(json, root)) {
        return VersionInfo(
            root.get("major", 0).asInt(),
            root.get("minor", 0).asInt(),
            root.get("patch", 0).asInt(),
            root.get("build", 0).asInt(),
            root.get("channel", "stable").asString()
        );
    }
    return VersionInfo();
}
