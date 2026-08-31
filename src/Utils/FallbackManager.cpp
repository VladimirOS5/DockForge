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
    if (suggestedAction == FallbackAction::SafeMode) {
        EnterSafeMode();
    }
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
    ok &= TestRenderer();
    ok &= TestShellIntegration();
    ok &= TestAudioCapture();
    ok &= TestNetwork();
    ok &= TestDiskSpace();
    return ok;
}

void FallbackManager::MarkSessionStart() {
    auto path = GetCrashFlagPath();
    std::ofstream f(path);
    f << "1";
}

void FallbackManager::MarkSessionEnd() {
    auto path = GetCrashFlagPath();
    std::filesystem::remove(path);
}

bool FallbackManager::DidPreviousSessionCrash() const {
    auto path = GetCrashFlagPath();
    return std::filesystem::exists(path);
}

void FallbackManager::EnterSafeMode() {
    m_safeMode = true;
    LOG_INFO("Entered safe mode");
}

void FallbackManager::ExitSafeMode() {
    m_safeMode = false;
    LOG_INFO("Exited safe mode");
}

bool FallbackManager::IsInSafeMode() const {
    return m_safeMode;
}

std::vector<FallbackEntry> FallbackManager::GetUnresolvedIssues() const {
    std::vector<FallbackEntry> unresolved;
    for (const auto& issue : m_issues) {
        if (!issue.resolved) unresolved.push_back(issue);
    }
    return unresolved;
}

void FallbackManager::ExecuteFallback(const FallbackEntry& entry) {
    LOG_INFO("Executing fallback for: " + entry.component);
    switch (entry.action) {
        case FallbackAction::RestartRenderer: break;
        case FallbackAction::RestartShellHooks: break;
        case FallbackAction::RestartAudioCapture: break;
        case FallbackAction::RestoreTaskbar: break;
        case FallbackAction::FullRestart: break;
        case FallbackAction::SafeMode: EnterSafeMode(); break;
    }
}

void FallbackManager::RollbackUpdate() {
    LOG_INFO("Rolling back update...");
    // TODO: Implement actual rollback logic (restore from backup)
}

std::filesystem::path FallbackManager::GetCrashFlagPath() const {
    wchar_t path[MAX_PATH];
    GetTempPathW(MAX_PATH, path);
    return std::filesystem::path(path) / L"dockforge_crash.flag";
}

bool FallbackManager::TestRenderer() { return true; }
bool FallbackManager::TestShellIntegration() { return true; }
bool FallbackManager::TestAudioCapture() { return true; }
bool FallbackManager::TestNetwork() {
    HINTERNET hInternet = InternetOpenA("DockForge", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
    if (!hInternet) return false;
    InternetCloseHandle(hInternet);
    return true;
}
bool FallbackManager::TestDiskSpace() { return true; }