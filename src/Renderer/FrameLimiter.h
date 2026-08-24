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
    float GetCurrentFPS() const { return m_currentFPS; }
    bool ShouldSkipFrame() const { return m_skipFrame; }
private:
    int m_targetFPS = 60;
    bool m_adaptive = true;
    bool m_vsync = true;
    bool m_skipFrame = false;
    std::chrono::steady_clock::time_point m_frameStart;
    std::chrono::steady_clock::time_point m_lastFrameEnd;
    float m_currentFPS = 60.0f;
    int m_frameCount = 0;
    std::chrono::steady_clock::time_point m_fpsUpdateTime;
    int m_consecutiveIdleFrames = 0;
    static constexpr int IDLE_THRESHOLD = 5;
};
