#pragma once
#include <string>
#include <vector>
#include <functional>
#include <filesystem>

enum class FallbackAction {
    RestartRenderer,
    RestartShellHooks,
    RestartAudioCapture,
    RestoreTaskbar,
    FullRestart,
    SafeMode
};

struct FallbackEntry {
    std::string component;
    std::string error;
    FallbackAction action;
    int retryCount = 0;
    bool resolved = false;
};

struct HealthCheckComponent {
    std::string name;
    std::function<bool()> check;
    bool lastHealthy = true;
    int consecutiveFailures = 0;
};

class FallbackManager {
public:
    static FallbackManager& Instance();
    void EnterSafeMode();
    void ExitSafeMode();
    bool IsSafeMode() const;
    void RegisterComponent(const std::string& name, std::function<bool()> healthCheck);
    void ReportError(const std::string& component, const std::string& error, FallbackAction suggestedAction);
    bool RunHealthChecks();
    bool ExecuteFallback(const FallbackEntry& entry);
    bool RunSelfTests();
    std::vector<FallbackEntry> GetUnresolvedIssues() const;
    void MarkSessionStart();
    void MarkSessionClean();
    bool DidPreviousSessionCrash();
    void RollbackUpdate();
    bool IsNetworkAvailable() const;
    bool TestRenderer();
    bool TestShellIntegration();
    bool TestAudioCapture();
    bool TestNetwork();
    bool TestDiskSpace();
private:
    FallbackManager() = default;
    std::vector<HealthCheckComponent> m_components;
    std::vector<FallbackEntry> m_issues;
    bool m_safeMode = false;
    bool m_sessionClean = false;
    std::vector<std::pair<std::string, std::function<bool()>>> m_healthChecks;
};
