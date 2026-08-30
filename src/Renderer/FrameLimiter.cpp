#include "FrameLimiter.h"
#include <thread>

FrameLimiter::FrameLimiter()
    : m_targetFrameTime(1000.0 / 60.0)
    , m_lastFrameTime(std::chrono::steady_clock::now())
    , m_lastFPSUpdate(std::chrono::steady_clock::now()) {}

void FrameLimiter::SetTargetFPS(int fps) {
    m_targetFPS = fps;
    m_targetFrameTime = 1000.0 / fps;
}

void FrameLimiter::BeginFrame() {
    m_frameStart = std::chrono::steady_clock::now();
}

void FrameLimiter::EndFrame() {
    auto now = std::chrono::steady_clock::now();
    m_frameCount++;

    auto elapsed = std::chrono::duration<double, std::milli>(now - m_lastFPSUpdate).count();
    if (elapsed >= 1000.0) {
        m_currentFPS = static_cast<float>(m_frameCount * 1000.0 / elapsed);
        m_frameCount = 0;
        m_lastFPSUpdate = now;
    }

    auto frameTime = std::chrono::duration<double, std::milli>(now - m_frameStart).count();
    if (frameTime < m_targetFrameTime) {
        auto sleepTime = std::chrono::milliseconds(static_cast<int>(m_targetFrameTime - frameTime));
        std::this_thread::sleep_for(sleepTime);
    }
    m_lastFrameTime = now;
}

bool FrameLimiter::ShouldSkipFrame() {
    if (!m_adaptive) return false;
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<double, std::milli>(now - m_frameStart).count();
    return elapsed < m_targetFrameTime;
}

float FrameLimiter::GetFrameTimeMs() const {
    auto now = std::chrono::steady_clock::now();
    return static_cast<float>(std::chrono::duration<double, std::milli>(now - m_lastFrameTime).count());
}
