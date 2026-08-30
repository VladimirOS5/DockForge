#include <windows.h>
#include "OTAUpdater.h"
#include "../Utils/Logger.h"
#include "../Utils/Config.h"
#include <wininet.h>
#include <wincrypt.h>
#include <sstream>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

OTAUpdater& OTAUpdater::Instance() {
    static OTAUpdater instance;
    return instance;
}

OTAUpdater::OTAUpdater() = default;
OTAUpdater::~OTAUpdater() { CancelOperation(); }

void OTAUpdater::SetUpdateUrl(const std::string& url) { m_updateUrl = url; }
void OTAUpdater::SetChannel(const std::string& channel) { m_channel = channel; }
void OTAUpdater::SetAutoCheckInterval(int minutes) { m_checkInterval = minutes; }
void OTAUpdater::SetAutoDownload(bool enabled) { m_autoDownload = enabled; }
void OTAUpdater::SetAutoInstall(bool enabled) { m_autoInstall = enabled; }

void OTAUpdater::SetProgressCallback(std::function<void(const UpdateProgress&)> cb) { m_progressCallback = cb; }
void OTAUpdater::SetCompletionCallback(std::function<void(bool, const std::string&)> cb) { m_completionCallback = cb; }

void OTAUpdater::Update(float deltaTime) {
    m_checkTimer += deltaTime;
    if (m_checkTimer >= m_checkInterval * 60.0f) {
        m_checkTimer = 0.0f;
        CheckForUpdateAsync();
    }
}

void OTAUpdater::CheckForUpdateAsync() {
    if (m_checking) return;
    m_checking = true;
    if (m_progressCallback) {
        UpdateProgress progress;
        progress.state = UpdateState::Checking;
        m_progressCallback(progress);
    }
    std::thread([this]() {
        auto result = CheckForUpdateInternal();
        m_checking = false;
        if (m_completionCallback) m_completionCallback(result.first, result.second);
    }).detach();
}

std::pair<bool, std::string> OTAUpdater::CheckForUpdateInternal() {
    return { false, "No update available" };
}

bool OTAUpdater::IsUpdatePending() const { return m_updatePending; }
void OTAUpdater::CancelOperation() {}
void OTAUpdater::CleanupOldInstallers() {}
void OTAUpdater::InstallUpdate() {}

UpdateProgress OTAUpdater::GetProgress() const {
    UpdateProgress p;
    p.state = UpdateState::Idle;
    return p;
}
