#pragma once
#include <string>
#include <chrono>
#include <windows.h>

enum class PerformanceProfile {
    PowerSaver,
    Balanced,
    Performance,
    Ultra,
    Adaptive
};

class PerformanceProfileManager {
public:
    static PerformanceProfileManager& Instance();
    void SetProfile(PerformanceProfile profile);
    PerformanceProfile GetProfile() const { return m_profile; }
    void AutoDetect();
    bool IsSystemIdle() const;
    bool IsFullscreenAppRunning() const;
    bool ShouldReduceEffects() const;
    bool ShouldPauseRendering() const;
    int GetTargetFPS() const;
    void Update(float deltaTime);
    void EnterSafeMode();
    void ExitSafeMode();
private:
    PerformanceProfile m_profile = PerformanceProfile::Balanced;
    bool m_safeMode = false;
    bool m_paused = false;
    bool m_fullscreen = false;
    bool m_idle = false;
    DWORD m_lastInputTick = 0;
    static constexpr DWORD IDLE_THRESHOLD_MS = 300000;
};
