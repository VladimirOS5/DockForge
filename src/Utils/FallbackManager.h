#pragma once
#include <string>
#include <functional>
#include <vector>

enum class FallbackAction {
    None,
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

class FallbackManager {
public:
    static FallbackManager& Instance();

    // Register a component for monitoring
    void RegisterComponent(const std::string& name, std::function<bool()> healthCheck);

    // Report an error
    void ReportError(const std::string& component, const std::string& error, FallbackAction suggestedAction);

    // Run health checks on all components
    void RunHealthChecks();

    // Execute fallback action
    bool ExecuteFallback(const FallbackEntry& entry);

    // Get all unresolved issues
    std::vector<FallbackEntry> GetUnresolvedIssues() const;

    // Safe mode: disable all effects, use minimal config
    void EnterSafeMode();
    bool IsInSafeMode() const { return m_safeMode; }
    void ExitSafeMode();

    // Crash recovery: check if previous session crashed
    bool DidPreviousSessionCrash();
    void MarkSessionStart();
    void MarkSessionClean();

    // Self-test suite
    bool RunSelfTests();
    bool TestRenderer();
    bool TestShellIntegration();
    bool TestAudioCapture();
    bool TestNetwork();
    bool TestDiskSpace();

private:
    FallbackManager() = default;

    struct ComponentHealth {
        std::string name;
        std::function<bool()> check;
        bool lastHealthy = true;
        int consecutiveFailures = 0;
    };

    std::vector<ComponentHealth> m_components;
    std::vector<FallbackEntry> m_issues;
    bool m_safeMode = false;

    std::filesystem::path GetCrashFlagPath() const;
};
