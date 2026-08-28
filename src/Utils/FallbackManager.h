#pragma once
#include <string>
#include <vector>
#include <functional>
#include <filesystem>  // FIX: added for std::filesystem::path

class FallbackManager {
public:
    static FallbackManager& Instance();
    void EnterSafeMode();
    bool IsSafeMode() const;
    void RegisterComponent(const std::string& name, std::function<bool()> healthCheck);
    bool RunHealthChecks();
    bool RunSelfTests();
    void MarkSessionStart();
    void MarkSessionClean();
    bool DidPreviousSessionCrash() const;
    void RollbackUpdate();
    bool IsNetworkAvailable() const;
private:
    FallbackManager() = default;
    bool m_safeMode = false;
    bool m_sessionClean = false;
    std::vector<std::pair<std::string, std::function<bool()>>> m_healthChecks;
    std::filesystem::path GetCrashFlagPath() const;
};
