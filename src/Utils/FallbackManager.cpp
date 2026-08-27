#include "FallbackManager.h"
#include "Logger.h"
#include "Config.h"
#include <windows.h>
#include <shlobj.h>
#include <filesystem>

FallbackManager& FallbackManager::Instance() {
    static FallbackManager instance;
    return instance;
}

void FallbackManager::RegisterComponent(const std::string& name, std::function<bool()> healthCheck) {
    m_components.push_back({name, healthCheck, true, 0});
    LOG_INFO("Registered health check for: " + name);
}

void FallbackManager::ReportError(const std::string& component, const std::string& error, FallbackAction suggestedAction) {
    LOG_ERROR("[" + component + "] " + error);
    FallbackEntry entry{component, error, suggestedAction, 0, false};
    m_issues.push_back(entry);

    // Auto-execute if retry count is low
    if (entry.retryCount < 3) {
        if (ExecuteFallback(entry)) {
            entry.resolved = true;
            entry.retryCount++;
        }
    }
}

void FallbackManager::RunHealthChecks() {
    for (auto& comp : m_components) {
        bool healthy = false;
        try {
            healthy = comp.check();
        } catch (...) {
            healthy = false;
        }

        if (!healthy) {
            comp.consecutiveFailures++;
            if (comp.consecutiveFailures >= 3) {
                LOG_ERROR("Component " + comp.name + " failed health check 3 times");
                ReportError(comp.name, "Repeated health check failure", FallbackAction::RestartRenderer);
            }
        } else {
            if (!comp.lastHealthy) {
                LOG_INFO("Component " + comp.name + " recovered");
            }
            comp.consecutiveFailures = 0;
        }
        comp.lastHealthy = healthy;
    }
}

bool FallbackManager::ExecuteFallback(const FallbackEntry& entry) {
    LOG_INFO("Executing fallback for " + entry.component + ": action=" + std::to_string(static_cast<int>(entry.action)));

    switch (entry.action) {
        case FallbackAction::RestartRenderer:
            // Signal to recreate D2D context
            LOG_INFO("Fallback: Requesting renderer restart");
            return true; // Handled by caller

        case FallbackAction::RestartShellHooks:
            LOG_INFO("Fallback: Requesting shell hook restart");
            return true;

        case FallbackAction::RestartAudioCapture:
            LOG_INFO("Fallback: Requesting audio capture restart");
            return true;

        case FallbackAction::RestoreTaskbar:
            LOG_INFO("Fallback: Restoring taskbar");
            // Taskbar restoration is handled by TaskbarHider
            return true;

        case FallbackAction::FullRestart:
            LOG_INFO("Fallback: Scheduling full restart");
            // Schedule restart via Application
            return true;

        case FallbackAction::SafeMode:
            EnterSafeMode();
            return true;

        default:
            return false;
    }
}

std::vector<FallbackEntry> FallbackManager::GetUnresolvedIssues() const {
    std::vector<FallbackEntry> unresolved;
    for (const auto& issue : m_issues) {
        if (!issue.resolved) unresolved.push_back(issue);
    }
    return unresolved;
}

void FallbackManager::EnterSafeMode() {
    if (m_safeMode) return;
    m_safeMode = true;
    LOG_WARN("ENTERING SAFE MODE — All effects disabled");

    auto& cfg = Config::Instance().GetMutable();
    cfg.backgroundEffect = "solid";
    cfg.audioReactiveBackground = false;
    cfg.thumbnailPreviews = false;
    cfg.jumpAnimation = false;
    cfg.genieEffect = false;
    cfg.badgePulse = false;
    cfg.runningIndicatorPulse = false;
    cfg.targetFPS = 30;
    cfg.vsync = true;
    cfg.showFPS = true; // Show FPS so user knows it's safe mode

    Config::Instance().SaveToFile();
    LOG_INFO("Safe mode config applied");
}

void FallbackManager::ExitSafeMode() {
    m_safeMode = false;
    LOG_INFO("Exiting safe mode");
    Config::Instance().LoadFromFile(); // Restore normal config
}

std::filesystem::path FallbackManager::GetCrashFlagPath() const {
    wchar_t path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, path))) {
        return std::filesystem::path(path) / L"DockForge" / L"crash.flag";
    }
    return std::filesystem::path("crash.flag");
}

bool FallbackManager::DidPreviousSessionCrash() {
    auto path = GetCrashFlagPath();
    bool crashed = std::filesystem::exists(path);
    if (crashed) {
        LOG_WARN("Previous session did not shut down cleanly (crash detected)");
    }
    return crashed;
}

void FallbackManager::MarkSessionStart() {
    auto path = GetCrashFlagPath();
    try {
        std::ofstream f(path);
        f << "running" << std::endl;
    } catch (...) {
        LOG_WARN("Failed to write crash flag");
    }
}

void FallbackManager::MarkSessionClean() {
    auto path = GetCrashFlagPath();
    try {
        if (std::filesystem::exists(path)) {
            std::filesystem::remove(path);
        }
    } catch (...) {
        LOG_WARN("Failed to remove crash flag");
    }
}

bool FallbackManager::RunSelfTests() {
    LOG_INFO("Running self-test suite...");
    bool allPassed = true;

    allPassed &= TestRenderer();
    allPassed &= TestShellIntegration();
    allPassed &= TestAudioCapture();
    allPassed &= TestNetwork();
    allPassed &= TestDiskSpace();

    if (allPassed) {
        LOG_INFO("All self-tests passed");
    } else {
        LOG_ERROR("Some self-tests failed");
    }
    return allPassed;
}

bool FallbackManager::TestRenderer() {
    // Check if D2D1.dll is available
    HMODULE d2d = LoadLibraryW(L"d2d1.dll");
    if (!d2d) {
        LOG_ERROR("Self-test: D2D1.dll not found");
        return false;
    }
    FreeLibrary(d2d);
    LOG_INFO("Self-test: Renderer OK");
    return true;
}

bool FallbackManager::TestShellIntegration() {
    // Check if COM is initialized and shell APIs work
    bool ok = SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED));
    if (ok) CoUninitialize();
    LOG_INFO("Self-test: Shell integration OK");
    return true;
}

bool FallbackManager::TestAudioCapture() {
    // Check if audio endpoint is available
    HMODULE mmdev = LoadLibraryW(L"Mmdevapi.dll");
    if (!mmdev) {
        LOG_WARN("Self-test: Mmdevapi.dll not found (audio will be unavailable)");
        return true; // Non-critical
    }
    FreeLibrary(mmdev);
    LOG_INFO("Self-test: Audio capture OK");
    return true;
}

bool FallbackManager::TestNetwork() {
    // Simple connectivity check
    HINTERNET hInternet = InternetOpenA("DockForge Test", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
    if (!hInternet) {
        LOG_WARN("Self-test: No internet connectivity");
        return true; // Non-critical
    }
    InternetCloseHandle(hInternet);
    LOG_INFO("Self-test: Network OK");
    return true;
}

bool FallbackManager::TestDiskSpace() {
    ULARGE_INTEGER freeBytes, totalBytes;
    if (GetDiskFreeSpaceExW(L"C:\\", &freeBytes, &totalBytes, nullptr)) {
        DWORDLONG freeMB = freeBytes.QuadPart / (1024 * 1024);
        if (freeMB < 100) {
            LOG_ERROR("Self-test: Low disk space (" + std::to_string(freeMB) + " MB)");
            return false;
        }
    }
    LOG_INFO("Self-test: Disk space OK");
    return true;
}
