#include "AnimatedIcon.h"

AnimatedIcon::AnimatedIcon() {
    m_lastFrameTime = std::chrono::steady_clock::now();
}

void AnimatedIcon::SetIcon(const LoadedIcon& icon) {
    m_icon = icon;
    m_isAnimated = icon.isAnimated && icon.frameCount > 1;
    m_currentFrame = 0;
    m_lastFrameTime = std::chrono::steady_clock::now();
}

void AnimatedIcon::Update() {
    if (!m_isAnimated || m_icon.frames.empty()) return;

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastFrameTime).count();

    if (elapsed >= static_cast<long long>(m_icon.frameDuration)) {
        m_currentFrame = (m_currentFrame + 1) % m_icon.frameCount;
        m_lastFrameTime = now;
    }
}

ID2D1Bitmap* AnimatedIcon::GetCurrentFrame() const {
    if (m_icon.frames.empty()) return nullptr;
    if (m_isAnimated) {
        return m_icon.frames[m_currentFrame].Get();
    }
    return m_icon.frames[0].Get();
}

void AnimatedIcon::Reset() {
    m_currentFrame = 0;
    m_lastFrameTime = std::chrono::steady_clock::now();
}
