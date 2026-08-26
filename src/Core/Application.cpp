#include "Application.h"
#include "../Utils/Logger.h"
#include "../Utils/Config.h"
#include "../Utils/Theme.h"
#include "../Utils/PerformanceProfile.h"
#include "../Core/MonitorManager.h"
#include "../Shell/ShellHookManager.h"
#include "../Shell/WindowManager.h"
#include "../Shell/TrayIconManager.h"
#include "../Settings/SettingsWindow.h"
#include "../Plugin/PluginManager.h"
#include <shlobj.h>
#include <filesystem>

Application::Application() {}
Application::~Application() { Shutdown(); }

bool Application::Initialize(HINSTANCE hInstance) {
    Config::Instance().LoadDefaults();
    Config::Instance().LoadFromFile();

    std::string profileStr = Config::Instance().Get().performanceProfile;
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
    LOG_INFO("  Chat 09 - Multi-Monitor & Performance");
    LOG_INFO("========================================");

    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) LOG_WARN("CoInitializeEx failed, continuing...");

    ThemeManager::Instance().Refresh();

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

    m_initialized = true;
    LOG_INFO("DockForge initialized.");
    return true;
}

int Application::Run() {
    if (!m_initialized) { LOG_FATAL("Application not initialized"); return 1; }
    LOG_INFO("Entering main message loop");
    
    MSG msg;
    auto lastTime = std::chrono::steady_clock::now();
    m_running = true;
    
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

        MonitorManager::Instance().UpdateAll(deltaTime);
        PluginManager::Instance().Update(deltaTime);
        TrayIconManager::Instance().Refresh();
        
        MonitorManager::Instance().RenderAll();
    }
    
    return 0;
}

void Application::Shutdown() {
    LOG_INFO("Shutting down DockForge...");
    m_running = false;
    PluginManager::Instance().Shutdown();
    MonitorManager::Instance().Shutdown();
    SettingsWindow::Instance().Destroy();
    TrayIconManager::Instance().Shutdown();
    WindowManager::Instance().Shutdown();
    ShellHookManager::Instance().Shutdown();
    CoUninitialize();
    if (m_taskbarHider) {
        if (!m_taskbarHider->Restore()) LOG_ERROR("Failed to restore taskbar during shutdown!");
        m_taskbarHider.reset();
    }
    LOG_INFO("Shutdown complete");
    LOG_INFO("========================================");
}