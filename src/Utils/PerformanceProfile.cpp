#include "PerformanceProfile.h"
#include "Config.h"
#include "Logger.h"

PerformanceProfileManager& PerformanceProfileManager::Instance() {
    static PerformanceProfileManager instance;
    return instance;
}

void PerformanceProfileManager::SetProfile(PerformanceProfile profile) {
    m_profile = profile;
    auto& cfg = Config::Instance().GetMutable();
    switch (profile) {
        case PerformanceProfile::Eco:
            cfg.targetFPS = 30;
            cfg.vsync = false;
            cfg.adaptiveFPS = true;
            cfg.idleFPS = 5;
            cfg.fullscreenFPS = 1;
            cfg.backgroundEffect = "solid";
            cfg.jumpAnimation = false;
            cfg.genieEffect = false;
            cfg.badgePulse = false;
            cfg.thumbnailPreviews = false;
            cfg.runningIndicatorPulse = false;
            cfg.showFPS = false;
            break;
        case PerformanceProfile::Balanced:
            cfg.targetFPS = 60;
            cfg.vsync = true;
            cfg.adaptiveFPS = true;
            cfg.idleFPS = 10;
            cfg.fullscreenFPS = 1;
            cfg.backgroundEffect = "acrylic";
            cfg.jumpAnimation = true;
            cfg.genieEffect = true;
            cfg.badgePulse = true;
            cfg.thumbnailPreviews = true;
            cfg.runningIndicatorPulse = true;
            break;
        case PerformanceProfile::Performance:
            cfg.targetFPS = 144;
            cfg.vsync = true;
            cfg.adaptiveFPS = false;
            cfg.idleFPS = 30;
            cfg.fullscreenFPS = 10;
            cfg.backgroundEffect = "liquidglass";
            cfg.jumpAnimation = true;
            cfg.genieEffect = true;
            cfg.badgePulse = true;
            cfg.thumbnailPreviews = true;
            cfg.runningIndicatorPulse = true;
            break;
        case PerformanceProfile::Custom:
            break;
    }
    Config::Instance().SaveToFile();
    LOG_INFO("Performance profile set");
}

void PerformanceProfileManager::AutoDetect() {
    SetProfile(PerformanceProfile::Balanced);
}

bool PerformanceProfileManager::IsFullscreenAppRunning() const {
    HWND fg = GetForegroundWindow();
    if (!fg) return false;
    RECT rcWindow, rcMonitor;
    GetWindowRect(fg, &rcWindow);
    HMONITOR hMon = MonitorFromWindow(fg, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = { sizeof(mi) };
    if (!GetMonitorInfo(hMon, &mi)) return false;
    rcMonitor = mi.rcMonitor;
    return (rcWindow.left <= rcMonitor.left && rcWindow.top <= rcMonitor.top &&
            rcWindow.right >= rcMonitor.right && rcWindow.bottom >= rcMonitor.bottom);
}

bool PerformanceProfileManager::IsSystemIdle() const {
    LASTINPUTINFO lii = { sizeof(lii) };
    if (GetLastInputInfo(&lii)) {
        DWORD idle = GetTickCount() - lii.dwTime;
        return idle > IDLE_THRESHOLD_MS;
    }
    return false;
}

void PerformanceProfileManager::Update(float deltaTime) {
    bool wasPaused = m_paused;
    m_fullscreen = IsFullscreenAppRunning();
    m_idle = IsSystemIdle();
    m_paused = m_fullscreen || m_idle;
    
    if (wasPaused != m_paused) {
        LOG_INFO(m_paused ? "Rendering paused" : "Rendering resumed");
    }
}

bool PerformanceProfileManager::ShouldPauseRendering() const {
    return m_paused;
}

bool PerformanceProfileManager::ShouldReduceEffects() const {
    return m_profile == PerformanceProfile::Eco || m_fullscreen;
}