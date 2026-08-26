#pragma once
#include <string>

enum class PerformanceProfile { Eco, Balanced, Performance, Custom };

class PerformanceProfileManager {
public:
    static PerformanceProfileManager& Instance();
    void SetProfile(PerformanceProfile profile);
    PerformanceProfile GetProfile() const { return m_profile; }
    void AutoDetect();
    bool IsFullscreenAppRunning() const;
    bool IsSystemIdle() const;
    void Update(float deltaTime);
    bool ShouldPauseRendering() const;
    bool ShouldReduceEffects() const;
private:
    PerformanceProfileManager() = default;
    PerformanceProfile m_profile = PerformanceProfile::Balanced;
    bool m_paused = false;
    bool m_fullscreen = false;
    bool m_idle = false;
    DWORD m_lastInputTick = 0;
    static constexpr DWORD IDLE_THRESHOLD_MS = 300000; // 5 min
};