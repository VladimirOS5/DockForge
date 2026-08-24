#include "FrameLimiter.h"
#include "../Utils/Logger.h"
#include <thread>
#include <windows.h>

FrameLimiter::FrameLimiter() {
    m_frameStart = std::chrono::steady_clock::now();
    m_lastFrameEnd = m_frameStart;
    m_fpsUpdateTime = m_frameStart;
}

void FrameLimiter::SetTargetFPS(int fps) {
    m_targetFPS = fps > 0 ? fps : 60;
}

void FrameLimiter::BeginFrame() {
    m_frameStart = std::chrono::steady_clock::now();
    m_skipFrame = false;
}

void FrameLimiter::EndFrame() {
    auto frameEnd = std::chrono::steady_clock::now();
    auto frameDuration = std::chrono::duration_cast<std::chrono::microseconds>(frameEnd - m_frameStart).count();

    // FPS counter
    m_frameCount++;
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(frameEnd - m_fpsUpdateTime).count();
    if (elapsed >= 1000) {
        m_currentFPS = m_frameCount * 1000.0f / elapsed;
        m_frameCount = 0;
        m_fpsUpdateTime = frameEnd;
    }

    // Adaptive: if frame took < 2ms and no input, consider idle
    if (m_adaptive && frameDuration < 2000) {
        m_consecutiveIdleFrames++;
        if (m_consecutiveIdleFrames > IDLE_THRESHOLD) {
            m_skipFrame = true;
        }
    } else {
        m_consecutiveIdleFrames = 0;
    }

    // V-Sync / target FPS wait
    if (m_vsync) {
        // Use DwmFlush for V-Sync on Windows
        DwmFlush();
    } else {
        int targetMicros = 1000000 / m_targetFPS;
        int sleepMicros = targetMicros - static_cast<int>(frameDuration);
        if (sleepMicros > 1000) {
            std::this_thread::sleep_for(std::chrono::microseconds(sleepMicros));
        }
    }

    m_lastFrameEnd = std::chrono::steady_clock::now();
}
