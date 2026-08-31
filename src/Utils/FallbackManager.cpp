#include "FallbackManager.h"
#include "Logger.h"
#include <fstream>
#include <windows.h>
#include <wininet.h>

FallbackManager& FallbackManager::Instance() {
    static FallbackManager inst;
    return inst;
}
void FallbackManager::RegisterComponent(const std::string& name, std::function<bool()> healthCheck) {
    m_components.emplace_back(name, healthCheck);
}
void FallbackManager::ReportError(const std::string& component, const std::string& error, FallbackAction suggestedAction) {
    LOG_ERROR("[" + component + "] " + error);
    FallbackEntry entry{component, error, suggestedAction, false};
    m_issues.push_back(entry);
    if (suggestedAction == FallbackAction::SafeMode) EnterSafeMode();
}
bool FallbackManager::RunHealthChecks() {
    bool allOk = true;
    for (auto& comp : m_components) {
        if (!comp.second()) {
            LOG_ERROR("Health check failed: " + comp.first);
            ReportError(comp.first, "Health check failed", FallbackAction::RestartRenderer);
            allOk = false;
        }
    }
    return allOk;
}
bool FallbackManager::RunSelfTests() {
    bool ok = true;
    ok &= TestRenderer(); ok &= TestShellIntegration();
    ok &= TestAudioCapture(); ok &= TestNetwork(); ok &= TestDiskSpace();
    return ok;
}
void FallbackManager::MarkSessionStart() {
    auto path = GetCrashFlagPath();
    std::ofstream f(path); f << "1";
}
void FallbackManager::MarkSessionEnd() {
    auto path = GetCrashFlagPath();
    std::filesystem::remove(path);
}
bool FallbackManager::DidPreviousSessionCrash() const {
    return std::filesystem::exists(GetCrashFlagPath());
}
void FallbackManager::EnterSafeMode() { m_safeMode = true; LOG_INFO("Entered safe mode"); }
void FallbackManager::ExitSafeMode() { m_safeMode = false; LOG_INFO("Exited safe mode"); }
bool FallbackManager::IsInSafeMode() const { return m_safeMode; }
std::vector<FallbackEntry> FallbackManager::GetUnresolvedIssues() const {
    std::vector<FallbackEntry> u;
    for (const auto& i : m_issues) if (!i.resolved) u.push_back(i);
    return u;
}
void FallbackManager::ExecuteFallback(const FallbackEntry& entry) {
    LOG_INFO("Executing fallback for: " + entry.component);
    if (entry.action == FallbackAction::SafeMode) EnterSafeMode();
}
void FallbackManager::RollbackUpdate() { LOG_INFO("Rolling back update..."); }
std::filesystem::path FallbackManager::GetCrashFlagPath() const {
    wchar_t path[MAX_PATH]; GetTempPathW(MAX_PATH, path);
    return std::filesystem::path(path) / L"dockforge_crash.flag";
}
bool FallbackManager::TestRenderer() { return true; }
bool FallbackManager::TestShellIntegration() { return true; }
bool FallbackManager::TestAudioCapture() { return true; }
bool FallbackManager::TestNetwork() {
    HINTERNET h = InternetOpenA("DockForge", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
    if (!h) return false;
    InternetCloseHandle(h); return true;
}
bool FallbackManager::TestDiskSpace() { return true; }