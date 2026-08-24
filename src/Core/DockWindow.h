#pragma once
#include <windows.h>
#include "../Renderer/D2DRenderer.h"
#include "../Renderer/EffectRenderer.h"
#include "../Renderer/Effects.h"
#include "../Renderer/FrameLimiter.h"
#include "../Renderer/IconLoader.h"
#include "../Renderer/TextureAtlas.h"
#include "../Renderer/AnimatedIcon.h"
#include "../Renderer/BadgeRenderer.h"
#include "../Utils/Config.h"
#include <memory>
#include <vector>
#include <string>

struct DockIcon {
    std::wstring appPath;
    std::wstring displayName;
    LoadedIcon loadedIcon;
    AnimatedIcon animator;
    AtlasEntry atlasEntry;
    int badgeCount = 0;
    bool isPinned = true;
    bool isRunning = false;
    float targetScale = 1.0f;
    float currentScale = 1.0f;
    float x = 0, y = 0;
    float width = 48, height = 48;
};

class DockWindow {
public:
    DockWindow();
    ~DockWindow();
    bool Create(HINSTANCE hInstance);
    void Show();
    void Hide();
    void Destroy();
    void RunMessageLoop();
    HWND GetHwnd() const { return m_hwnd; }
    bool IsRunning() const { return m_running; }
    void RequestQuit() { m_running = false; }
private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);
    void OnPaint();
    void OnSize(UINT width, UINT height);
    void UpdateWindowPosition();
    void CycleEffect();
    void DrawBackgroundEffect();
    void DrawIcons();
    void DrawFPS();
    void LoadDemoIcons();
    void UpdateAnimations(float deltaTime);
    void UpdateIconPositions();

    HWND m_hwnd = nullptr;
    HINSTANCE m_hInstance = nullptr;
    std::unique_ptr<D2DRenderer> m_renderer;
    std::unique_ptr<EffectRenderer> m_effectRenderer;
    std::unique_ptr<BlurEffect> m_blur;
    std::unique_ptr<AcrylicEffect> m_acrylic;
    std::unique_ptr<LiquidGlassEffect> m_liquidGlass;
    std::unique_ptr<FrameLimiter> m_frameLimiter;
    std::unique_ptr<IconLoader> m_iconLoader;
    std::unique_ptr<TextureAtlas> m_textureAtlas;
    std::unique_ptr<BadgeRenderer> m_badgeRenderer;
    std::vector<DockIcon> m_icons;

    bool m_running = true;
    float m_animTime = 0.0f;
    int m_currentEffect = 1;
    int m_hoveredIcon = -1;

    static constexpr int DOCK_HEIGHT = 64;
    static constexpr int DOCK_MARGIN = 12;
    static constexpr int DOCK_CORNER_RADIUS = 16;
    static constexpr int ICON_SIZE = 48;
    static constexpr int ICON_PADDING = 8;
};
