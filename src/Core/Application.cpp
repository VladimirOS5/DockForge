#include "Application.h"
#include "../Utils/Logger.h"
#include "../Utils/Config.h"
#include "../Utils/Theme.h"
#include "../Utils/PerformanceProfile.h"
#include "../Utils/FallbackManager.h"
#include "../Updater/OTAUpdater.h"
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

    // Safe mode check
    if (cfg.safeMode) {
        FallbackManager::Instance().EnterSafeMode();
    }

    // Crash recovery
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
    LOG_INFO("  DockForge v1.0.0-alpha");
    LOG_INFO("  Chat 11 - OTA, Installer & Fallback");
    LOG_INFO("========================================");
    LOG_INFO("Version: " + SemanticVersion::Current().ToString());
    LOG_INFO("Channel: " + cfg.updateChannel);
    LOG_INFO("Safe mode: " + std::string(cfg.safeMode ? "YES" : "NO"));

    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) LOG_WARN("CoInitializeEx failed, continuing...");

    ThemeManager::Instance().Refresh();

    // Self-tests
    if (cfg.runSelfTestsOnStart) {
        RunSelfTests();
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

    auto taskbarHider = std::make_unique<TaskbarHider>();
    if (!taskbarHider->Hide()) LOG_WARN("Failed to hide taskbar, continuing with overlay mode...");
    taskbarHider.release(); // Managed by MonitorManager or stays hidden

    if (!SettingsWindow::Instance().Create(hInstance)) {
        LOG_WARN("Failed to create settings window");
    }

    MonitorManager::Instance().Initialize(hInstance);

    // OTA initialization
    InitializeOTA();
    InitializeFallback();

    m_initialized = true;
    LOG_INFO("DockForge initialized successfully.");
    return true;
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
            case UpdateState::Checking:
                LOG_INFO("OTA: Checking for updates...");
                break;
            case UpdateState::UpdateAvailable:
                LOG_INFO("OTA: Update available - " + progress.targetVersion.ToString());
                break;
            case UpdateState::Downloading:
                LOG_INFO("OTA: Downloading update... " + std::to_string(static_cast<int>(progress.downloadPercent)) + "%");
                break;
            case UpdateState::Verified:
                LOG_INFO("OTA: Update downloaded and verified");
                break;
            case UpdateState::Error:
                LOG_ERROR("OTA Error [" + std::to_string(static_cast<int>(progress.error)) + "]: " + progress.errorDetails);
                break;
            default:
                break;
        }
    });

    updater.SetCompletionCallback([this](bool success, const std::string& message) {
        if (success && OTAUpdater::Instance().IsUpdatePending()) {
            m_updatePending = true;
            LOG_INFO("OTA: " + message);
            ShowUpdateNotification();
        }
    });

    // Cleanup old installers
    updater.CleanupOldInstallers();

    // Initial check after 30 seconds
    m_otaTimer = -30.0f;
    LOG_INFO("OTA Updater initialized");
}

void Application::InitializeFallback() {
    FallbackManager::Instance().RegisterComponent("Renderer", []() {
        // Check if D2D is functional
        return true; // Placeholder
    });
    FallbackManager::Instance().RegisterComponent("ShellHooks", []() {
        return ShellHookManager::Instance().IsInitialized();
    });
    FallbackManager::Instance().RegisterComponent("AudioCapture", []() {
        // Check audio endpoint
        return true; // Placeholder
    });
    LOG_INFO("Fallback manager initialized with 3 health checks");
}

void Application::RunSelfTests() {
    bool passed = FallbackManager::Instance().RunSelfTests();
    if (!passed) {
        LOG_WARN("Some self-tests failed. DockForge may have limited functionality.");
    }
}

void Application::ShowUpdateNotification() {
    auto progress = OTAUpdater::Instance().GetProgress();
    std::wstring msg = L"DockForge update " + 
        std::wstring(progress.targetVersion.ToString().begin(), progress.targetVersion.ToString().end()) +
        L" is ready to install.";
    // In real implementation, this would show a custom toast or tray notification
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

        // OTA background checks
        UpdateOTA(deltaTime);

        // Health checks every 10 seconds
        static float healthTimer = 0;
        healthTimer += deltaTime;
        if (healthTimer >= 10.0f) {
            healthTimer = 0;
            FallbackManager::Instance().RunHealthChecks();
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
    } else if (m_otaTimer < 0) {
        // Initial delay counting up from negative
        if (m_otaTimer >= 0) {
            OTAUpdater::Instance().CheckForUpdateAsync();
        }
    }
}

void Application::Shutdown() {
    LOG_INFO("Shutting down DockForge...");
    m_running = false;

    OTAUpdater::Instance().CancelOperation();

    PluginManager::Instance().Shutdown();
    MonitorManager::Instance().Shutdown();
    SettingsWindow::Instance().Destroy();
    TrayIconManager::Instance().Shutdown();
    WindowManager::Instance().Shutdown();
    ShellHookManager::Instance().Shutdown();

    // Restore taskbar
    auto taskbarHider = std::make_unique<TaskbarHider>();
    if (!taskbarHider->Restore()) LOG_ERROR("Failed to restore taskbar during shutdown!");

    CoUninitialize();

    FallbackManager::Instance().MarkSessionClean();

    LOG_INFO("Shutdown complete");
    LOG_INFO("========================================");
}
