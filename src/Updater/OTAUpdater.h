#pragma once
#include "VersionInfo.h"
#include <string>
#include <functional>
#include <filesystem>
#include <thread>
#include <atomic>

enum class UpdateState {
    Idle,
    Checking,
    UpdateAvailable,
    Downloading,
    Downloaded,
    Verifying,
    Verified,
    Installing,
    Installed,
    Error,
    RolledBack
};

enum class UpdateError {
    None,
    NetworkError,
    ParseError,
    DownloadFailed,
    ChecksumMismatch,
    InstallFailed,
    NoWritePermission,
    DiskFull,
    WindowsVersionUnsupported
};

struct UpdateProgress {
    UpdateState state = UpdateState::Idle;
    UpdateError error = UpdateError::None;
    float downloadPercent = 0.0f;
    std::string statusMessage;
    SemanticVersion targetVersion;
    std::string errorDetails;
};

class OTAUpdater {
public:
    static OTAUpdater& Instance();

    // Configuration
    void SetUpdateUrl(const std::string& url) { m_updateUrl = url; }
    void SetChannel(const std::string& channel) { m_channel = channel; } // "stable", "beta", "alpha"
    void SetAutoCheckInterval(int minutes) { m_checkIntervalMinutes = minutes; }
    void SetAutoDownload(bool enabled) { m_autoDownload = enabled; }
    void SetAutoInstall(bool enabled) { m_autoInstall = enabled; }

    // Actions
    void CheckForUpdateAsync();
    void DownloadUpdateAsync();
    void InstallUpdate();
    void RollbackUpdate();
    void CancelOperation();

    // Queries
    UpdateProgress GetProgress() const;
    bool IsOperationInProgress() const;
    bool IsUpdatePending() const;
    std::filesystem::path GetPendingInstallerPath() const;

    // Callbacks
    using ProgressCallback = std::function<void(const UpdateProgress&)>;
    using CompletionCallback = std::function<void(bool success, const std::string& message)>;
    void SetProgressCallback(ProgressCallback cb) { m_progressCallback = cb; }
    void SetCompletionCallback(CompletionCallback cb) { m_completionCallback = cb; }

    // Background checker (call from main loop)
    void Update(float deltaTime);

    // Cleanup old installers
    void CleanupOldInstallers();

private:
    OTAUpdater() = default;
    ~OTAUpdater();

    void CheckWorker();
    void DownloadWorker();
    bool VerifyChecksum(const std::filesystem::path& file, const std::string& expectedSha256);
    bool IsWindowsVersionSupported(const std::string& minVersion);
    void SetState(UpdateState state, UpdateError error = UpdateError::None, const std::string& details = "");
    void NotifyProgress();

    std::string BuildUpdateUrl() const;
    bool ParseReleaseJson(const std::string& json, ReleaseInfo& out);
    bool DownloadFile(const std::string& url, const std::filesystem::path& dest, 
                      std::atomic<bool>& cancel, float& progress);

    std::string m_updateUrl = "https://api.dockforge.app/v1/releases";
    std::string m_channel = "stable";
    int m_checkIntervalMinutes = 60;
    bool m_autoDownload = true;
    bool m_autoInstall = false;

    UpdateProgress m_progress;
    ReleaseInfo m_pendingRelease;
    std::filesystem::path m_installerPath;
    std::filesystem::path m_backupPath;

    std::thread m_workerThread;
    std::atomic<bool> m_cancelFlag{false};
    std::atomic<bool> m_operationInProgress{false};

    ProgressCallback m_progressCallback;
    CompletionCallback m_completionCallback;

    float m_checkTimer = 0.0f;
    bool m_initialized = false;
};
