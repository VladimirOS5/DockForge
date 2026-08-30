#pragma once
#include <chrono>
#include <thread>

class FrameLimiter {
public:
    FrameLimiter();
    void SetTargetFPS(int fps);
    void SetAdaptive(bool adaptive) { m_adaptive = adaptive; }
    void BeginFrame();
    void EndFrame();
    bool ShouldSkipFrame();
    float GetCurrentFPS() const { return m_currentFPS; }
    float GetFrameTimeMs() const;
private:
    int m_targetFPS = 60;
    double m_targetFrameTime = 16.666;
    bool m_adaptive = false;
    std::chrono::steady_clock::time_point m_frameStart;
    std::chrono::steady_clock::time_point m_lastFrameTime;
    std::chrono::steady_clock::time_point m_lastFPSUpdate;
    float m_currentFPS = 0.0f;
    int m_frameCount = 0;
};
