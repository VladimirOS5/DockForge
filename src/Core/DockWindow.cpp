#include "DockWindow.h"
#include "../Utils/Logger.h"
#include "../Utils/ScreenCapture.h"
#include "../Utils/Theme.h"
#include "MonitorManager.h"
#include <windowsx.h>
#include <chrono>
#include <cmath>

DockWindow::DockWindow() {}
DockWindow::~DockWindow() { Destroy(); }

void DockWindow::Destroy() { if (m_hwnd) { DestroyWindow(m_hwnd); m_hwnd = nullptr; } }

bool DockWindow::Create(HINSTANCE hInstance, const MonitorInfo* monitor) {
    m_hInstance = hInstance;
    m_monitor = monitor;
    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc = DockWindow::WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"DockForgeDockWindow";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    if (!RegisterClassExW(&wc)) { LOG_ERROR("Failed to register window class"); return false; }

    RECT workArea;
    if (monitor) {
        workArea = monitor->workArea;
    } else {
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);
    }
    int screenWidth = workArea.right - workArea.left;
    int dockWidth = screenWidth - (DOCK_MARGIN * 2);
    int x = workArea.left + DOCK_MARGIN;
    int y = workArea.bottom - DOCK_HEIGHT - DOCK_MARGIN;

    // FIX: Removed WS_EX_TRANSPARENT — it blocks mouse input!
    m_hwnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TOPMOST,
        L"DockForgeDockWindow", L"DockForge", WS_POPUP,
        x, y, dockWidth, DOCK_HEIGHT, nullptr, nullptr, hInstance, this);
    if (!m_hwnd) { LOG_ERROR("Failed to create dock window"); return false; }

    SetLayeredWindowAttributes(m_hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);

    m_renderer = std::make_unique<D2DRenderer>();
    if (!m_renderer->Initialize(m_hwnd)) { LOG_ERROR("Failed to initialize D2D renderer"); return false; }

    m_iconLoader = std::make_unique<IconLoader>();
    m_iconLoader->Initialize(m_renderer->GetRenderTarget());

    m_textureAtlas = std::make_unique<TextureAtlas>();
    m_textureAtlas->Initialize(m_renderer->GetRenderTarget(), 1024);

    m_badgeRenderer = std::make_unique<BadgeRenderer>();
    m_badgeRenderer->Initialize(m_renderer->GetRenderTarget(), m_renderer->GetWriteFactory());

    m_effectRenderer = std::make_unique<EffectRenderer>();
    if (m_effectRenderer->Initialize()) {
        m_effectRenderer->Resize(dockWidth, DOCK_HEIGHT);
        m_blur = std::make_unique<BlurEffect>();
        m_blur->Initialize(m_effectRenderer->GetContext());
        m_acrylic = std::make_unique<AcrylicEffect>();
        m_acrylic->Initialize(m_effectRenderer->GetContext(), dockWidth, DOCK_HEIGHT);
        m_liquidGlass = std::make_unique<LiquidGlassEffect>();
        m_liquidGlass->Initialize(m_effectRenderer->GetContext(), dockWidth, DOCK_HEIGHT);
    }

    m_audioReactive = std::make_unique<AudioReactiveEffect>();

    m_frameLimiter = std::make_unique<FrameLimiter>();
    auto& cfg = Config::Instance().Get();
    m_frameLimiter->SetTargetFPS(cfg.targetFPS);
    m_frameLimiter->SetAdaptive(cfg.adaptiveFPS);
    m_frameLimiter->SetVSync(cfg.vsync);

    if (cfg.audioReactiveBackground) {
        m_currentEffect = 3;
        m_audioReactive->Initialize();
    } else if (cfg.backgroundEffect == "acrylic") {
        m_currentEffect = 1;
    } else if (cfg.backgroundEffect == "liquidGlass") {
        m_currentEffect = 2;
    } else {
        m_currentEffect = 0;
    }

    LoadDemoIcons();
    if (cfg.showStartButton) LoadStartButton();
    UpdateTrayIcons();
    UpdateIconPositions();
    UpdateRunningStates();
    StartSlideIn();

    TrayIconManager::Instance().SetCallback([this]() {
        UpdateTrayIcons();
        UpdateIconPositions();
    });

    LOG_INFO("Dock window created");
    return true;
}

void DockWindow::LoadStartButton() {
    DockIcon icon;
    icon.type = IconType::StartButton;
    icon.appPath = L"StartMenu";
    icon.displayName = L"Start";
    icon.isPinned = true;
    icon.isRunning = false;

    std::wstring skinPath = StartButton::GetSkinIconPath(Config::Instance().Get().startButtonSkin);
    size_t comma = skinPath.find_last_of(L',');
    if (comma != std::wstring::npos) {
        std::wstring path = skinPath.substr(0, comma);
        icon.loadedIcon = m_iconLoader->LoadFromExe(path, ICON_SIZE);
    } else {
        icon.loadedIcon = m_iconLoader->LoadSystemIcon(L"shell:::{48e7caab-b918-4a58-a94d-5053194fdb18}", ICON_SIZE);
    }

    if (!icon.loadedIcon.frames.empty()) {
        icon.animator.SetIcon(icon.loadedIcon);
        AtlasEntry entry;
        if (m_textureAtlas->AddBitmap(icon.loadedIcon.frames[0].Get(), entry)) icon.atlasEntry = entry;
    }
    m_icons.insert(m_icons.begin(), std::move(icon));
}

void DockWindow::LoadDemoIcons() {
    struct DemoApp { std::wstring path; std::wstring exePath; int badge; };
    std::vector<DemoApp> demoApps = {
        {L"explorer.exe", L"C:/Windows/explorer.exe", 3},
        {L"notepad.exe", L"C:/Windows/notepad.exe", 0},
        {L"calc.exe", L"C:/Windows/System32/calc.exe", 0},
        {L"msedge.exe", L"C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe", 5},
        {L"ms-settings:", L"", 0},
    };

    for (auto& app : demoApps) {
        DockIcon icon;
        icon.type = IconType::App;
        icon.appPath = app.path;
        icon.exePath = app.exePath;
        icon.isPinned = true;
        icon.badgeCount = app.badge;
        icon.scale = 1.0f; icon.jumpOffsetY = 0.0f; icon.jumpScale = 1.0f; icon.jumpOpacity = 1.0f;
        icon.genieSkew = 0.0f; icon.genieScaleY = 1.0f; icon.genieOpacity = 1.0f;
        icon.badgeScale = 1.0f; icon.indicatorOpacity = 1.0f;

        size_t pos = app.path.find_last_of(L"\\/");
        icon.processName = (pos != std::wstring::npos) ? app.path.substr(pos + 1) : app.path;

        if (app.path.find(L':') != std::wstring::npos) {
            icon.loadedIcon = m_iconLoader->LoadSystemIcon(app.path, ICON_SIZE);
        } else {
            wchar_t sysPath[MAX_PATH];
            GetSystemDirectoryW(sysPath, MAX_PATH);
            icon.loadedIcon = m_iconLoader->LoadFromExe(std::wstring(sysPath) + L"\\" + app.path, ICON_SIZE);
        }

        if (!icon.loadedIcon.frames.empty()) {
            icon.animator.SetIcon(icon.loadedIcon);
            AtlasEntry entry;
            if (m_textureAtlas->AddBitmap(icon.loadedIcon.frames[0].Get(), entry)) icon.atlasEntry = entry;
        }
        m_icons.push_back(std::move(icon));
    }
    LOG_INFO("Loaded " + std::to_string(m_icons.size()) + " demo icons");
}

void DockWindow::UpdateTrayIcons() {
    auto& cfg = Config::Instance().Get();
    m_icons.erase(std::remove_if(m_icons.begin(), m_icons.end(),
        [](const DockIcon& i) { return i.type == IconType::Tray; }), m_icons.end());

    if (!cfg.showTrayIcons) return;

    for (const auto& tray : TrayIconManager::Instance().GetIcons()) {
        DockIcon icon;
        icon.type = IconType::Tray;
        icon.trayInfo = tray;
        icon.tooltip = tray.tooltip;
        icon.isPinned = false;
        icon.isRunning = true;
        icon.scale = 1.0f;

        if (tray.hIcon) {
            icon.loadedIcon = m_iconLoader->LoadFromHICON(tray.hIcon, ICON_SIZE);
            if (!icon.loadedIcon.frames.empty()) {
                icon.animator.SetIcon(icon.loadedIcon);
                AtlasEntry entry;
                if (m_textureAtlas->AddBitmap(icon.loadedIcon.frames[0].Get(), entry)) icon.atlasEntry = entry;
            }
        }
        m_icons.push_back(std::move(icon));
    }
}

void DockWindow::UpdateRunningStates() {
    for (auto& icon : m_icons) {
        if (icon.type != IconType::App) continue;
        int count = WindowManager::Instance().GetWindowCountForProcess(icon.processName);
        bool wasRunning = icon.isRunning;
        icon.isRunning = (count > 0);
        icon.windowCount = count;
        if (wasRunning != icon.isRunning && icon.isRunning) {
            icon.indicatorOpacity = 0.0f;
            TweenEngine::Instance().AddTween(&icon.indicatorOpacity, 0.0f, 1.0f, 300.0f, EasingType::EaseOut);
        }
    }
}

void DockWindow::UpdateIconPositions() {
    RECT rc; GetClientRect(m_hwnd, &rc);
    float width = static_cast<float>(rc.right - rc.left);
    float height = static_cast<float>(rc.bottom - rc.top);

    size_t appCount = 0, trayCount = 0;
    for (const auto& icon : m_icons) {
        if (icon.type == IconType::Tray) ++trayCount;
        else ++appCount;
    }

    float totalAppWidth = appCount * ICON_SIZE + (appCount > 0 ? (appCount - 1) * ICON_PADDING : 0);
    float totalTrayWidth = trayCount * ICON_SIZE + (trayCount > 0 ? (trayCount - 1) * ICON_PADDING : 0);
    float trayStartX = width - totalTrayWidth - 12.0f;
    float startX = (width - totalAppWidth - totalTrayWidth) / 2.0f;
    if (startX + totalAppWidth >= trayStartX - 12.0f) startX = 12.0f;

    float iconY = (height - ICON_SIZE) / 2.0f;
    float currentX = startX;

    for (auto& icon : m_icons) {
        if (icon.type == IconType::Tray) continue;
        icon.baseX = currentX; icon.baseY = iconY;
        icon.width = ICON_SIZE; icon.height = ICON_SIZE;
        currentX += ICON_SIZE + ICON_PADDING;
    }

    currentX = trayStartX;
    for (auto& icon : m_icons) {
        if (icon.type != IconType::Tray) continue;
        icon.baseX = currentX; icon.baseY = iconY;
        icon.width = ICON_SIZE; icon.height = ICON_SIZE;
        currentX += ICON_SIZE + ICON_PADDING;
    }
}

void DockWindow::Show() { if (m_hwnd) { ShowWindow(m_hwnd, SW_SHOW); UpdateWindow(m_hwnd); } }
void DockWindow::Hide() { if (m_hwnd) ShowWindow(m_hwnd, SW_HIDE); }

void DockWindow::StartSlideIn() {
    auto& cfg = Config::Instance().Get();
    m_slideOffsetY = static_cast<float>(DOCK_HEIGHT + DOCK_MARGIN);
    m_slideOpacity = 0.0f;
    m_slideInComplete = false;
    float speed = cfg.slideAnimationSpeed * cfg.animationSpeed;
    TweenEngine::Instance().AddTween(&m_slideOffsetY, m_slideOffsetY, 0.0f, speed, EasingFromString(cfg.defaultEasing));
    TweenEngine::Instance().AddTween(&m_slideOpacity, 0.0f, 1.0f, speed * 0.8f, EasingType::EaseOut, [this]() {
        m_slideInComplete = true;
    });
}

void DockWindow::StartSlideOut() {
    auto& cfg = Config::Instance().Get();
    float speed = cfg.slideAnimationSpeed * cfg.animationSpeed;
    TweenEngine::Instance().AddTween(&m_slideOffsetY, 0.0f, static_cast<float>(DOCK_HEIGHT + DOCK_MARGIN), speed, EasingFromString(cfg.defaultEasing));
    TweenEngine::Instance().AddTween(&m_slideOpacity, 1.0f, 0.0f, speed * 0.8f, EasingType::EaseOut);
}

void DockWindow::RunMessageLoop() {
    MSG msg;
    auto lastTime = std::chrono::steady_clock::now();
    while (m_running) {
        m_frameLimiter->BeginFrame();
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) { m_running = false; break; }
            TranslateMessage(&msg); DispatchMessageW(&msg);
        }
        if (!m_running) break;
        auto now = std::chrono::steady_clock::now();
        float deltaTime = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;
        m_animTime += deltaTime;

        PluginManager::Instance().Update(deltaTime);
        UpdateAnimations(deltaTime);
        if (m_liquidGlass) m_liquidGlass->UpdateAnimation(deltaTime);
        if (m_audioReactive) m_audioReactive->Update(deltaTime);
        if (!m_frameLimiter->ShouldSkipFrame()) OnPaint();
        m_frameLimiter->EndFrame();
    }
}
