#pragma once
#include <string>
#include <vector>
#include <functional>
#include <filesystem>

enum class FallbackAction { RestartRenderer, RestartShellHooks, RestartAudioCapture, RestoreTaskbar, FullRestart, SafeMode };

struct FallbackEntry {
    std::string component;
    std::string error;
    FallbackAction action;
    bool resolved = false;
};

class FallbackManager {
public:
    static FallbackManager& Instance();
    void RegisterComponent(const std::string& name, std::function<bool()> healthCheck);
    void ReportError(const std::string& component, const std::string& error, FallbackAction suggestedAction);
    bool RunHealthChecks();
    bool RunSelfTests();
    void MarkSessionStart();
    void MarkSessionEnd();
    bool DidPreviousSessionCrash() const;
    void EnterSafeMode();
    void ExitSafeMode();
    bool IsInSafeMode() const;
    std::vector<FallbackEntry> GetUnresolvedIssues() const;
    void ExecuteFallback(const FallbackEntry& entry);
private:
    std::vector<std::pair<std::string, std::function<bool()>>> m_components;
    std::vector<FallbackEntry> m_issues;
    bool m_safeMode = false;
    std::filesystem::path GetCrashFlagPath() const;
};
