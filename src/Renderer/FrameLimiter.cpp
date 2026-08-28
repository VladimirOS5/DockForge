#include "FrameLimiter.h"
#include "../Utils/Logger.h"
#include <windows.h>
#include <dwmapi.h>  // FIX: added for DwmFlush
#include <cmath>

FrameLimiter::FrameLimiter() : m_targetFrameTime(1000.0f / 60.0f), m_lastFrameTime(0) {}

void FrameLimiter::SetTargetFPS(int fps) {
    m_targetFPS = fps;
    m_targetFrameTime = 1000.0 / fps;
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
    double timeSinceLastUpdate = std::chrono::duration<double, std::milli>(now - m_lastFPSUpdate).count();
    if (timeSinceLastUpdate >= 1000.0) {
        m_currentFPS = static_cast<float>(m_frameCount * 1000.0 / timeSinceLastUpdate);
        m_frameCount = 0;
        m_lastFPSUpdate = now;
    }

    if (m_vsync) {
        DwmFlush();
    } else {
        double sleepTime = m_targetFrameTime - elapsed;
        if (sleepTime > 1.0) {
            Sleep(static_cast<DWORD>(sleepTime));
        }
    }
}

bool FrameLimiter::ShouldSkipFrame() {
    if (!m_adaptive) return false;
    auto now = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double, std::milli>(now - m_frameStart).count();
    return elapsed < m_targetFrameTime * 0.5;
}

float FrameLimiter::GetCurrentFPS() const {
    return m_currentFPS;
}

float FrameLimiter::GetFrameTimeMs() const {
    return static_cast<float>(m_lastFrameTime);
}
