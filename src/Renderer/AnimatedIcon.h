#pragma once
#include "IconLoader.h"
#include <chrono>

class AnimatedIcon {
public:
    AnimatedIcon();
    void SetIcon(const LoadedIcon& icon);
    void Update();
    ID2D1Bitmap* GetCurrentFrame() const;
    int GetCurrentFrameIndex() const { return m_currentFrame; }
    bool IsAnimated() const { return m_isAnimated; }
    void Reset();
private:
    LoadedIcon m_icon;
    int m_currentFrame = 0;
    std::chrono::steady_clock::time_point m_lastFrameTime;
    bool m_isAnimated = false;
};
