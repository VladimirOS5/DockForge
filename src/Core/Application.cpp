#include "Application.h"
#include "MainWindow.h"
#include "../Utils/Logger.h"
#include "../Utils/Config.h"
#include "../Utils/Theme.h"
#include "../Utils/PerformanceProfile.h"
#include "../Utils/FallbackManager.h"
#include "../Utils/MemoryTracker.h"
#include "../Updater/OTAUpdater.h"
#include "../Renderer/D2DRenderer.h"
#include "../Renderer/FrameLimiter.h"
#include "../Shell/ShellHookManager.h"
#include "../Shell/WindowManager.h"
#include "../Shell/TrayIconManager.h"
#include "../Shell/TaskbarHider.h"
#include "../Core/DockWindow.h"
#include "../Settings/SettingsWindow.h"
#include "../Plugin/PluginManager.h"
#include <shlobj.h>
#include <shellapi.h>
#include <filesystem>

Application::Application() = default;
Application::~Application() = default;

bool Application::Initialize(HINSTANCE hInstance) {
    m_hInstance = hInstance;
    Logger::Initialize();
    LOG_INFO("DockForge v" + VersionInfo::GetVersionString() + " starting...");

    if (FallbackManager::Instance().DidPreviousSessionCrash()) {
        LOG_WARN("Previous session crashed. Entering safe mode.");
        Config::Instance().GetMutable().safeMode = true;
    }

    InitializeDPI();
    Config::Instance().LoadFromFile();
    Theme::Instance().Apply(Config::Instance().Get().theme);
    PerformanceProfileManager::Instance().AutoDetect();
    InitializeMemoryTracking();
    InitializeFallback();

    m_taskbarHider = std::make_unique<TaskbarHider>();
    m_taskbarHider->Hide();

    MonitorManager::Instance().Initialize(hInstance);
    ShellHookManager::Instance().Initialize();
    WindowManager::Instance().Initialize();
    TrayIconManager::Instance().Initialize(hInstance);
    SettingsWindow::Instance().Create(hInstance);
    PluginManager::Instance().Initialize();
    InitializeOTA();

    LogSystemInfo();
    RunSelfTests();
    m_initialized = true;
    return true;
}

int Application::Run() {
    FrameLimiter limiter;
    limiter.SetTargetFPS(PerformanceProfileManager::Instance().GetTargetFPS());
    limiter.SetAdaptive(true);

    MSG msg = {};
    auto lastTime = std::chrono::high_resolution_clock::now();

    while (m_running) {
        limiter.BeginFrame();
        auto now = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;

        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) { m_running = false; break; }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        PerformanceProfileManager::Instance().Update(deltaTime);
        if (PerformanceProfileManager::Instance().ShouldPauseRendering()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        OTAUpdater::Instance().Update(deltaTime);
        MonitorManager::Instance().UpdateAll(deltaTime);
        MonitorManager::Instance().RenderAll();
        SettingsWindow::Instance().Update(deltaTime);
        PluginManager::Instance().Update(deltaTime);
        limiter.EndFrame();
    }
    return 0;
}

void Application::Shutdown() {
    LOG_INFO("Shutting down DockForge...");
    PluginManager::Instance().Shutdown();
    SettingsWindow::Instance().Destroy();
    TrayIconManager::Instance().Shutdown();
    WindowManager::Instance().Shutdown();
    ShellHookManager::Instance().Shutdown();
    MonitorManager::Instance().Shutdown();
    if (m_taskbarHider) m_taskbarHider->Show();
    Logger::Shutdown();
    m_initialized = false;
}

void Application::InstallPendingUpdate() {
    m_updatePending = true;
}

void Application::RunStabilityTests() {
    auto reports = StabilityTest::Instance().RunAll();
    StabilityTest::Instance().WriteReport(reports, "stability_report.txt");
}

void Application::PrintMemoryReport() {
    MemoryTracker::Instance().PrintReport();
}

void Application::InitializeOTA() {
    OTAUpdater::Instance().SetUpdateUrl("https://api.dockforge.app/updates");
    OTAUpdater::Instance().SetChannel("stable");
    OTAUpdater::Instance().SetAutoCheckInterval(60);
    OTAUpdater::Instance().SetAutoDownload(true);
    OTAUpdater::Instance().SetAutoInstall(false);
    OTAUpdater::Instance().SetProgressCallback([](const UpdateProgress& p) {
        LOG_INFO("OTA Progress: " + std::to_string(static_cast<int>(p.percent)) + "%");
    });
    OTAUpdater::Instance().SetCompletionCallback([this](bool success, const std::string& msg) {
        if (success) {
            LOG_INFO("Update ready: " + msg);
            m_updatePending = true;
        } else {
            LOG_INFO("No update: " + msg);
        }
    });
}

void Application::InitializeFallback() {
    FallbackManager::Instance().MarkSessionStart();
    FallbackManager::Instance().RegisterComponent("Renderer", []() { return true; });
    FallbackManager::Instance().RegisterComponent("ShellHooks", []() { return true; });
    FallbackManager::Instance().RegisterComponent("AudioCapture", []() { return true; });
}

void Application::RunSelfTests() {
    if (!FallbackManager::Instance().RunSelfTests()) {
        LOG_WARN("Some self-tests failed. Check fallback manager.");
    }
}

void Application::UpdateOTA(float deltaTime) {
    m_otaTimer += deltaTime;
}

void Application::ShowUpdateNotification() {}
void Application::InitializeMemoryTracking() {}
void Application::InitializeDPI() {}
void Application::LogSystemInfo() {}
void Application::RunLongTermStabilityTest() {}
