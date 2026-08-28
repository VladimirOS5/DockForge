#pragma once
#include <windows.h>  // FIX: added for DWORD, HWND, etc.
#include <string>

enum class PerformanceProfile { Balanced, Performance, Eco, Custom };

class PerformanceProfileManager {
public:
    static PerformanceProfileManager& Instance();
    void SetProfile(PerformanceProfile profile);
    PerformanceProfile GetProfile() const;
    void Update(float deltaTime);
    bool ShouldPauseRendering() const;
    bool IsBatterySaver() const;
    bool IsFullscreenAppRunning() const;
    float GetCurrentTargetFPS() const;
private:
    PerformanceProfileManager() = default;
    PerformanceProfile m_profile = PerformanceProfile::Balanced;
    float m_targetFPS = 60.0f;
    bool m_shouldPause = false;
    DWORD m_lastInputTick = 0;
    static constexpr DWORD IDLE_THRESHOLD_MS = 300000;
};
