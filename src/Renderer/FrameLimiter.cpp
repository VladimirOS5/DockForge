#include "FrameLimiter.h"

FrameLimiter::FrameLimiter() : m_targetFrameTime(1000.0f / 60.0f), m_lastFrameTime(0) {}

void FrameLimiter::SetTargetFPS(int fps) {
    m_targetFPS = fps;
    m_targetFrameTime = 1000.0 / static_cast<double>(fps);
}

void FrameLimiter::BeginFrame() {
    m_frameStart = std::chrono::high_resolution_clock::now();
}

void FrameLimiter::EndFrame() {
    auto frameEnd = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double, std::milli>(frameEnd - m_frameStart).count();
    m_lastFrameTime = elapsed;
    m_frameCount++;

    auto now = std::chrono::high_resolution_clock::now();
    if (std::chrono::duration<float>(now - m_lastFPSUpdate).count() >= 1.0f) {
        m_currentFPS = static_cast<float>(m_frameCount);
        m_frameCount = 0;
        m_lastFPSUpdate = now;
    }

    double targetMs = 1000.0 / m_targetFPS;
    if (elapsed < targetMs) {
        double sleepMs = targetMs - elapsed;
        auto sleepDuration = std::chrono::milliseconds(static_cast<long long>(sleepMs));
        std::this_thread::sleep_for(sleepDuration);
    }
}

bool FrameLimiter::ShouldSkipFrame() {
    if (!m_adaptive) return false;
    auto now = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double, std::milli>(now - m_frameStart).count();
    return elapsed < m_targetFrameTime * 0.5;
}

float FrameLimiter::GetFrameTimeMs() const {
    return static_cast<float>(m_lastFrameTime);
}
