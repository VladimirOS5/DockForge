#pragma once

enum class PerformanceProfile { Eco, Balanced, Performance, Custom };

class PerformanceProfileManager {
public:
    static PerformanceProfileManager& Instance();
    void SetProfile(PerformanceProfile profile);
    PerformanceProfile GetProfile() const;
    void AutoDetect();
    bool IsSystemIdle() const;
    bool ShouldReduceEffects() const;
    void Update(float deltaTime);
    bool ShouldPauseRendering() const;
    bool IsBatterySaver() const;
    float GetTargetFPS() const;
    void SetTargetFPS(float fps);
private:
    PerformanceProfile m_profile = PerformanceProfile::Balanced;
    float m_targetFPS = 60.0f;
    bool m_shouldPause = false;
    DWORD m_lastInputTick = 0;
    bool m_paused = false;
    bool m_fullscreen = false;
    bool m_idle = false;
    static constexpr DWORD IDLE_THRESHOLD_MS = 300000;
};
