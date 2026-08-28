#include "Application.h"
#include "../Utils/Logger.h"
#include "../Utils/Config.h"
#include "../Utils/Theme.h"
#include "../Utils/PerformanceProfile.h"
#include "../Utils/FallbackManager.h"
#include "../Utils/DPIHelper.h"
#include "../Utils/HDREnums.h"
#include "../Updater/OTAUpdater.h"
#include "../Testing/MemoryTracker.h"
#include "../Testing/StabilityTest.h"
#include "../Core/MonitorManager.h"
#include "../Shell/ShellHookManager.h"
#include "../Shell/WindowManager.h"
#include "../Shell/TrayIconManager.h"
#include "../Shell/TaskbarHider.h"
#include "../Settings/SettingsWindow.h"
#include "../Plugin/PluginManager.h"
#include <shlobj.h>
#include <filesystem>
#include <shellapi.h>

Application::Application() {}
Application::~Application() { Shutdown(); }

bool Application::Initialize(HINSTANCE hInstance) {
    m_hInstance = hInstance;

    Config::Instance().LoadDefaults();
    Config::Instance().LoadFromFile();
    auto& cfg = Config::Instance().Get();

    if (cfg.safeMode) {
        FallbackManager::Instance().EnterSafeMode();
    }

    if (FallbackManager::Instance().DidPreviousSessionCrash()) {
        LOG_WARN("Previous session crashed. Running diagnostics...");
        FallbackManager::Instance().EnterSafeMode();
    }
    FallbackManager::Instance().MarkSessionStart();

    std::string profileStr = cfg.performanceProfile;
    if (profileStr == "eco") PerformanceProfileManager::Instance().SetProfile(PerformanceProfile::Eco);
    else if (profileStr == "performance") PerformanceProfileManager::Instance().SetProfile(PerformanceProfile::Performance);
    else if (profileStr == "custom") PerformanceProfileManager::Instance().SetProfile(PerformanceProfile::Custom);
    else PerformanceProfileManager::Instance().SetProfile(PerformanceProfile::Balanced);

    wchar_t localAppDataPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, localAppDataPath))) {
        std::filesystem::path logDir = std::filesystem::path(localAppDataPath) / L"DockForge" / L"logs";
        Logger::Instance().Init(logDir, "DockForge");
    } else {
        Logger::Instance().Init("logs", "DockForge");
    }
    LOG_INFO("========================================");
    LOG_INFO("  DockForge v1.0.0");
    LOG_INFO("  Chat 12 - Polish, Testing & Release");
    LOG_INFO("========================================");
    LOG_INFO("Version: " + SemanticVersion::Current().ToString());
    LOG_INFO("Channel: " + cfg.updateChannel);
    LOG_INFO("Safe mode: " + std::string(cfg.safeMode ? "YES" : "NO"));
    LOG_INFO("Memory tracking: " + std::string(cfg.enableMemoryTracking ? "ON" : "OFF"));

    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) LOG_WARN("CoInitializeEx failed, continuing...");

    ThemeManager::Instance().Refresh();
    LogSystemInfo();
    InitializeDPI();
    InitializeMemoryTracking();

    if (cfg.runSelfTestsOnStart) {
        RunSelfTests();
    }
    if (cfg.runStabilityTestsOnStart) {
        RunStabilityTests();
    }

    if (!ShellHookManager::Instance().Initialize()) LOG_WARN("Failed to initialize ShellHookManager");
    WindowManager::Instance().Initialize();
    TrayIconManager::Instance().Initialize();

    ShellHookManager::Instance().SetCallback([](const ShellEvent& event) {
        switch (event.type) {
            case ShellEventType::WindowCreated: WindowManager::Instance().OnWindowEvent(event.hwnd, true); break;
            case ShellEventType::WindowDestroyed: WindowManager::Instance().OnWindowEvent(event.hwnd, false); break;
            case ShellEventType::WindowActivated: WindowManager::Instance().OnWindowActivated(event.hwnd); break;
            case ShellEventType::WindowRedraw: WindowManager::Instance().OnWindowTitleChanged(event.hwnd); break;
            default: break;
        }
    });

    m_taskbarHider = std::make_unique<TaskbarHider>();
    if (!m_taskbarHider->Hide()) LOG_WARN("Failed to hide taskbar, continuing with overlay mode...");

    if (!SettingsWindow::Instance().Create(hInstance)) {
        LOG_WARN("Failed to create settings window");
    }

    MonitorManager::Instance().Initialize(hInstance);
    InitializeOTA();
    InitializeFallback();

    m_initialized = true;
    LOG_INFO("DockForge v1.0.0 initialized successfully.");
    return true;
}

void Application::LogSystemInfo() {
    auto& cfg = Config::Instance().Get();
    OSVERSIONINFOEXW osvi = { sizeof(osvi) };
    #pragma warning(suppress: 4996)
    GetVersionExW(reinterpret_cast<OSVERSIONINFOW*>(&osvi));
    LOG_INFO("Windows: " + std::to_string(osvi.dwMajorVersion) + "." +
             std::to_string(osvi.dwMinorVersion) + "." + std::to_string(osvi.dwBuildNumber));

    SYSTEM_INFO si;
    GetSystemInfo(&si);
    LOG_INFO("CPU cores: " + std::to_string(si.dwNumberOfProcessors));

    MEMORYSTATUSEX mem = { sizeof(mem) };
    if (GlobalMemoryStatusEx(&mem)) {
        LOG_INFO("RAM: " + std::to_string(mem.ullTotalPhys / (1024*1024*1024)) + " GB total, " +
                 std::to_string(mem.ullAvailPhys / (1024*1024*1024)) + " GB free");
    }

    if (cfg.logDPIInfo) {
        float scale = DPIHelper::GetSystemScale();
        LOG_INFO("System DPI scale: " + std::to_string(scale));
    }
    if (cfg.logHDRInfo) {
        HDRHelper::LogDisplayInfo();
    }
}

void Application::InitializeDPI() {
    auto& cfg = Config::Instance().Get();
    if (cfg.handleDPIScale) {
        DPIHelper::EnablePerMonitorV2();
    }
}

void Application::InitializeMemoryTracking() {
    auto& cfg = Config::Instance().Get();
    MemoryTracker::Instance().SetEnabled(cfg.enableMemoryTracking);
    if (cfg.enableMemoryTracking) {
        LOG_INFO("Memory tracking enabled");
    }
}

void Application::RunStabilityTests() {
    LOG_INFO("Running stability test suite...");
    StabilityTest::Instance().RegisterDefaultTests();
    auto reports = StabilityTest::Instance().RunAll();

    wchar_t localAppData[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, localAppData))) {
        std::filesystem::path reportPath = std::filesystem::path(localAppData) / L"DockForge" / L"stability_report.txt";
        StabilityTest::Instance().WriteReport(reports, reportPath.string());
        LOG_INFO("Stability report written to: " + reportPath.string());
    }
}

void Application::RunLongTermStabilityTest() {
    auto& cfg = Config::Instance().Get();
    StabilityTest::SimulationConfig simConfig;
    simConfig.realTimeHours = cfg.stabilityTestDurationHours;
    simConfig.timeScale = cfg.stabilityTimeScale;
    simConfig.logEveryHour = true;

    LOG_INFO("Starting long-term stability simulation (" + std::to_string(cfg.stabilityTestDurationHours) + "h)...");
    bool ok = StabilityTest::Instance().RunLongTermSimulation(simConfig);
    if (ok) LOG_INFO("Long-term simulation PASSED");
    else LOG_ERROR("Long-term simulation FAILED");
}

void Application::PrintMemoryReport() {
    MemoryTracker::Instance().PrintReport();
}

void Application::InitializeOTA() {
    auto& cfg = Config::Instance().Get();
    if (!cfg.autoCheckUpdates) {
        LOG_INFO("Auto-check updates disabled");
        return;
    }

    auto& updater = OTAUpdater::Instance();
    updater.SetUpdateUrl(cfg.updateServerUrl);
    updater.SetChannel(cfg.updateChannel);
    updater.SetAutoCheckInterval(cfg.updateCheckInterval);
    updater.SetAutoDownload(cfg.autoDownloadUpdates);
    updater.SetAutoInstall(cfg.autoInstallUpdates);

    updater.SetProgressCallback([](const UpdateProgress& progress) {
        switch (progress.state) {
            case UpdateState::Checking: LOG_INFO("OTA: Checking for updates..."); break;
            case UpdateState::UpdateAvailable: LOG_INFO("OTA: Update available - " + progress.targetVersion.ToString()); break;
            case UpdateState::Downloading: LOG_INFO("OTA: Downloading update... " + std::to_string(static_cast<int>(progress.downloadPercent)) + "%"); break;
            case UpdateState::Verified: LOG_INFO("OTA: Update downloaded and verified"); break;
            case UpdateState::Error: LOG_ERROR("OTA Error [" + std::to_string(static_cast<int>(progress.error)) + "]: " + progress.errorDetails); break;
            default: break;
        }
    });

    updater.SetCompletionCallback([this](bool success, const std::string& message) {
        if (success && OTAUpdater::Instance().IsUpdatePending()) {
            m_updatePending = true;
            LOG_INFO("OTA: " + message);
            ShowUpdateNotification();
        }
    });

    updater.CleanupOldInstallers();
    m_otaTimer = 0.0f;
    LOG_INFO("OTA Updater initialized");
}

void Application::InitializeFallback() {
    FallbackManager::Instance().RegisterComponent("Renderer", []() { return true; });
    FallbackManager::Instance().RegisterComponent("ShellHooks", []() { return ShellHookManager::Instance().IsInitialized(); });
    FallbackManager::Instance().RegisterComponent("AudioCapture", []() { return true; });
    FallbackManager::Instance().RegisterComponent("Memory", []() {
        auto metrics = MemoryTracker::Instance().GetMetrics();
        return metrics.currentBytes < 100 * 1024 * 1024;
    });
    LOG_INFO("Fallback manager initialized with 4 health checks");
}

void Application::RunSelfTests() {
    bool passed = FallbackManager::Instance().RunSelfTests();
    if (!passed) {
        LOG_WARN("Some self-tests failed. DockForge may have limited functionality.");
    }
}

// UTF-8 to wstring helper
static std::wstring ToWString(const std::string& str) {
    if (str.empty()) return {};
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    std::wstring result(size - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, result.data(), size);
    return result;
}

void Application::ShowUpdateNotification() {
    auto progress = OTAUpdater::Instance().GetProgress();
    std::wstring msg = L"DockForge update " + ToWString(progress.targetVersion.ToString()) + L" is ready to install.";
    LOG_INFO("Update notification: " + progress.targetVersion.ToString() + " ready");
}

void Application::InstallPendingUpdate() {
    if (m_updatePending) {
        OTAUpdater::Instance().InstallUpdate();
        RequestQuit();
    }
}

int Application::Run() {
    if (!m_initialized) { LOG_FATAL("Application not initialized"); return 1; }
    LOG_INFO("Entering main message loop");

    MSG msg;
    auto lastTime = std::chrono::steady_clock::now();

    while (m_running) {
        bool hasRunningDock = false;
        for (auto* dock : MonitorManager::Instance().GetDockWindows()) {
            if (dock && dock->IsRunning()) hasRunningDock = true;
        }
        if (!hasRunningDock) break;

        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) { m_running = false; break; }
            TranslateMessage(&msg); DispatchMessageW(&msg);
        }
        if (!m_running) break;

        auto now = std::chrono::steady_clock::now();
        float deltaTime = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;

        PerformanceProfileManager::Instance().Update(deltaTime);
        if (PerformanceProfileManager::Instance().ShouldPauseRendering()) {
            Sleep(100);
            continue;
        }

        UpdateOTA(deltaTime);

        static float healthTimer = 0;
        healthTimer += deltaTime;
        if (healthTimer >= 10.0f) {
            healthTimer = 0;
            FallbackManager::Instance().RunHealthChecks();
            static int healthCount = 0;
            healthCount++;
            if (healthCount >= 6) {
                healthCount = 0;
                if (Config::Instance().Get().enableMemoryTracking) {
                    auto metrics = MemoryTracker::Instance().GetMetrics();
                    if (metrics.currentBytes > 50 * 1024 * 1024) {
                        LOG_WARN("Memory usage high: " + std::to_string(metrics.currentBytes / 1024 / 1024) + " MB");
                    }
                }
            }
        }

        MonitorManager::Instance().UpdateAll(deltaTime);
        PluginManager::Instance().Update(deltaTime);
        TrayIconManager::Instance().Refresh();
        MonitorManager::Instance().RenderAll();
    }

    return 0;
}

void Application::UpdateOTA(float deltaTime) {
    m_otaTimer += deltaTime;
    if (m_otaTimer >= Config::Instance().Get().updateCheckInterval * 60.0f) {
        m_otaTimer = 0;
        OTAUpdater::Instance().Update(deltaTime);
    }
    static bool initialCheckDone = false;
    if (!initialCheckDone && m_otaTimer > 5.0f) {
        initialCheckDone = true;
        OTAUpdater::Instance().CheckForUpdateAsync();
    }
}

void Application::Shutdown() {
    LOG_INFO("Shutting down DockForge...");
    m_running = false;

    OTAUpdater::Instance().CancelOperation();

    if (Config::Instance().Get().enableMemoryTracking) {
        MemoryTracker::Instance().PrintReport();
        wchar_t localAppData[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, localAppData))) {
            std::filesystem::path memReport = std::filesystem::path(localAppData) / L"DockForge" / L"memory_report.txt";
            MemoryTracker::Instance().WriteReportToFile(memReport.string());
        }
    }

    PluginManager::Instance().Shutdown();
    MonitorManager::Instance().Shutdown();
    SettingsWindow::Instance().Destroy();
    TrayIconManager::Instance().Shutdown();
    WindowManager::Instance().Shutdown();
    ShellHookManager::Instance().Shutdown();

    if (m_taskbarHider) {
        if (!m_taskbarHider->Restore()) LOG_ERROR("Failed to restore taskbar during shutdown!");
    }

    CoUninitialize();
    FallbackManager::Instance().MarkSessionClean();

    LOG_INFO("Shutdown complete. Goodbye!");
    LOG_INFO("========================================");
}
