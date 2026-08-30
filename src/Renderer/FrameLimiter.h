#pragma once
#include <chrono>

class FrameLimiter {
public:
    FrameLimiter();
    void SetTargetFPS(int fps);
    void SetAdaptive(bool enabled) { m_adaptive = enabled; }
    void SetVSync(bool enabled) { m_vsync = enabled; }
    void BeginFrame();
    void EndFrame();
    bool ShouldSkipFrame();
    float GetFrameTimeMs() const;
    float GetCurrentFPS() const { return m_currentFPS; }
private:
    int m_targetFPS = 60;
    bool m_adaptive = true;
    bool m_vsync = true;
    bool m_skipFrame = false;
    std::chrono::steady_clock::time_point m_frameStart;
    std::chrono::steady_clock::time_point m_lastFrameEnd;
    std::chrono::steady_clock::time_point m_lastFPSUpdate;
    float m_currentFPS = 60.0f;
    int m_frameCount = 0;
    double m_targetFrameTime = 1000.0 / 60.0;
    double m_lastFrameTime = 0.0;
    std::chrono::steady_clock::time_point m_fpsUpdateTime;
    int m_consecutiveIdleFrames = 0;
    static constexpr int IDLE_THRESHOLD = 5;
};
