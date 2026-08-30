#include "PerformanceProfile.h"
#include "Logger.h"
#include <windows.h>

PerformanceProfileManager& PerformanceProfileManager::Instance() {
    static PerformanceProfileManager inst;
    return inst;
}

void PerformanceProfileManager::SetProfile(PerformanceProfile profile) {
    m_profile = profile;
    LOG_INFO("Performance profile set to: " + std::to_string(static_cast<int>(profile)));
}

void PerformanceProfileManager::AutoDetect() {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    MEMORYSTATUSEX ms;
    ms.dwLength = sizeof(ms);
    GlobalMemoryStatusEx(&ms);
    if (ms.ullTotalPhys < 4ULL * 1024 * 1024 * 1024) {
        SetProfile(PerformanceProfile::PowerSaver);
    } else if (ms.ullTotalPhys < 8ULL * 1024 * 1024 * 1024) {
        SetProfile(PerformanceProfile::Balanced);
    } else {
        SetProfile(PerformanceProfile::Performance);
    }
}

bool PerformanceProfileManager::IsSystemIdle() const {
    LASTINPUTINFO lii = { sizeof(LASTINPUTINFO) };
    if (GetLastInputInfo(&lii)) {
        DWORD idle = GetTickCount() - lii.dwTime;
        return idle > IDLE_THRESHOLD_MS;
    }
    return false;
}

bool PerformanceProfileManager::IsFullscreenAppRunning() const {
    HWND fg = GetForegroundWindow();
    if (!fg) return false;
    RECT rcWindow, rcMonitor;
    GetWindowRect(fg, &rcWindow);
    HMONITOR hMon = MonitorFromWindow(fg, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = { sizeof(MONITORINFO) };
    if (GetMonitorInfo(hMon, &mi)) {
        rcMonitor = mi.rcMonitor;
        return (rcWindow.left == rcMonitor.left && rcWindow.top == rcMonitor.top &&
                rcWindow.right == rcMonitor.right && rcWindow.bottom == rcMonitor.bottom);
    }
    return false;
}

bool PerformanceProfileManager::ShouldReduceEffects() const {
    return m_profile == PerformanceProfile::PowerSaver || m_fullscreen || m_idle;
}

bool PerformanceProfileManager::ShouldPauseRendering() const {
    return m_paused;
}

int PerformanceProfileManager::GetTargetFPS() const {
    switch (m_profile) {
        case PerformanceProfile::PowerSaver: return 30;
        case PerformanceProfile::Balanced: return 60;
        case PerformanceProfile::Performance: return 120;
        case PerformanceProfile::Ultra: return 240;
        case PerformanceProfile::Adaptive: return m_idle ? 30 : 60;
    }
    return 60;
}

void PerformanceProfileManager::Update(float deltaTime) {
    m_fullscreen = IsFullscreenAppRunning();
    m_idle = IsSystemIdle();
    bool shouldPause = m_fullscreen || m_idle;
    if (shouldPause != m_paused) {
        m_paused = shouldPause;
        LOG_INFO("Rendering " + std::string(m_paused ? "paused" : "resumed"));
    }
}

void PerformanceProfileManager::EnterSafeMode() {
    m_safeMode = true;
    SetProfile(PerformanceProfile::PowerSaver);
    LOG_INFO("Entered safe mode");
}

void PerformanceProfileManager::ExitSafeMode() {
    m_safeMode = false;
    AutoDetect();
    LOG_INFO("Exited safe mode");
}
