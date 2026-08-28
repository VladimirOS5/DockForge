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
#include "../Renderer/TweenEngine.h"
#include "../Renderer/AudioReactiveEffect.h"
#include "../Utils/Config.h"
#include "../Shell/ShellHookManager.h"
#include "../Shell/WindowManager.h"
#include "../Shell/ThumbnailPreview.h"
#include "../Shell/JumpListManager.h"
#include "../Shell/TrayIconManager.h"
#include "../Shell/StartButton.h"
#include "../Shell/DockContextMenu.h"
#include "../Shell/TaskbarHider.h"
#include "../Plugin/PluginManager.h"
#include <memory>
#include <vector>
#include <string>

struct MonitorInfo;

enum class IconType { App, Tray, StartButton };

struct DockIcon {
    IconType type = IconType::App;
    std::wstring appPath;
    std::wstring displayName;
    std::wstring exePath;
    std::wstring processName;
    std::wstring tooltip;
    LoadedIcon loadedIcon;
    AnimatedIcon animator;
    AtlasEntry atlasEntry;
    TrayIconInfo trayInfo;
    int badgeCount = 0;
    bool isPinned = true;
    bool isRunning = false;
    int windowCount = 0;

    float scale = 1.0f;
    float jumpOffsetY = 0.0f;
    float jumpScale = 1.0f;
    float jumpOpacity = 1.0f;
    float genieSkew = 0.0f;
    float genieScaleY = 1.0f;
    float genieOpacity = 1.0f;
    float badgeScale = 1.0f;
    float indicatorOpacity = 1.0f;

    float baseX = 0, baseY = 0;
    float width = 48, height = 48;
    int tweenIdScale = -1;
    int tweenIdJump = -1;
    int tweenIdGenie = -1;
    int tweenIdBadge = -1;
};

class DockWindow {
public:
    DockWindow();
    ~DockWindow();
    bool Create(HINSTANCE hInstance, const MonitorInfo* monitor = nullptr);
    void Show();
    void Hide();
    void Destroy();
    void RunMessageLoop();
    void Update(float deltaTime);
    void Render();
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
    void LoadStartButton();
    void UpdateTrayIcons();
    void UpdateAnimations(float deltaTime);
    void UpdateIconPositions();
    void UpdateRunningStates();
    void SetIconHover(int index, bool hovered);
    void TriggerJump(int index);
    void TriggerGenie(int index);
    void TriggerBadgePulse(int index);
    void StartSlideIn();
    void StartSlideOut();

    void ShowThumbnailPreview(int iconIndex);
    void HideThumbnailPreview();
    void ShowContextMenu(int iconIndex, int x, int y);
    void ShowDockContextMenu(int x, int y);
    void LaunchOrActivate(int iconIndex);
    void LaunchApp(const DockIcon& icon);

    HWND m_hwnd = nullptr;
    HINSTANCE m_hInstance = nullptr;
    const MonitorInfo* m_monitor = nullptr;
    std::unique_ptr<D2DRenderer> m_renderer;
    std::unique_ptr<EffectRenderer> m_effectRenderer;
    std::unique_ptr<BlurEffect> m_blur;
    std::unique_ptr<AcrylicEffect> m_acrylic;
    std::unique_ptr<LiquidGlassEffect> m_liquidGlass;
    std::unique_ptr<AudioReactiveEffect> m_audioReactive;
    std::unique_ptr<FrameLimiter> m_frameLimiter;
    std::unique_ptr<IconLoader> m_iconLoader;
    std::unique_ptr<TextureAtlas> m_textureAtlas;
    std::unique_ptr<BadgeRenderer> m_badgeRenderer;
    std::unique_ptr<ThumbnailPreview> m_thumbnailPreview;
    std::vector<DockIcon> m_icons;

    bool m_running = true;
    float m_animTime = 0.0f;
    int m_currentEffect = 0;
    int m_hoveredIcon = -1;
    int m_previewIcon = -1;

    float m_slideOffsetY = 100.0f;
    float m_slideOpacity = 0.0f;
    bool m_slideInComplete = false;

    static constexpr int DOCK_HEIGHT = 64;
    static constexpr int DOCK_MARGIN = 12;
    static constexpr int DOCK_CORNER_RADIUS = 16;
    static constexpr int ICON_SIZE = 48;
    static constexpr int ICON_PADDING = 8;
};
