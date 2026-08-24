#include "Application.h"
#include "../Utils/Logger.h"
#include "../Utils/Config.h"
#include <shlobj.h>
#include <filesystem>

Application::Application() {}
Application::~Application() { Shutdown(); }

bool Application::Initialize(HINSTANCE hInstance) {
    Config::Instance().LoadDefaults();

    wchar_t localAppDataPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, localAppDataPath))) {
        std::filesystem::path logDir = std::filesystem::path(localAppDataPath) / L"DockForge" / L"logs";
        Logger::Instance().Init(logDir, "DockForge");
    } else {
        Logger::Instance().Init("logs", "DockForge");
    }
    LOG_INFO("========================================");
    LOG_INFO("  DockForge v1.0.0-alpha");
    LOG_INFO("  Chat 03 - Icons & Resources");
    LOG_INFO("========================================");

    m_taskbarHider = std::make_unique<TaskbarHider>();
    if (!m_taskbarHider->Hide()) {
        LOG_WARN("Failed to hide taskbar, continuing with overlay mode...");
    }

    m_dockWindow = std::make_unique<DockWindow>();
    if (!m_dockWindow->Create(hInstance)) {
        LOG_FATAL("Failed to create dock window");
        m_taskbarHider->Restore();
        return false;
    }
    m_dockWindow->Show();
    m_initialized = true;
    LOG_INFO("DockForge initialized. F1=cycle effects, Hover=magification, Badges active");
    return true;
}

int Application::Run() {
    if (!m_initialized) { LOG_FATAL("Application not initialized"); return 1; }
    LOG_INFO("Entering main message loop");
    m_dockWindow->RunMessageLoop();
    return 0;
}

void Application::Shutdown() {
    LOG_INFO("Shutting down DockForge...");
    if (m_dockWindow) { m_dockWindow->RequestQuit(); m_dockWindow->Hide(); m_dockWindow.reset(); }
    if (m_taskbarHider) {
        if (!m_taskbarHider->Restore()) LOG_ERROR("Failed to restore taskbar during shutdown!");
        m_taskbarHider.reset();
    }
    LOG_INFO("Shutdown complete");
    LOG_INFO("========================================");
}
