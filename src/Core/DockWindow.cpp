#include "DockWindow.h"
#include "../Utils/Logger.h"
#include "../Utils/ScreenCapture.h"
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

    m_hwnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TOPMOST | WS_EX_TRANSPARENT,
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

    m_frameLimiter = std::make_unique<FrameLimiter>();
    auto& cfg = Config::Instance().Get();
    m_frameLimiter->SetTargetFPS(cfg.targetFPS);
    m_frameLimiter->SetAdaptive(cfg.adaptiveFPS);
    m_frameLimiter->SetVSync(cfg.vsync);

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
        {L"explorer.exe", L"C:\\Windows\\explorer.exe", 3},
        {L"notepad.exe", L"C:\\Windows\\notepad.exe", 0},
        {L"calc.exe", L"C:\\Windows\\System32\\calc.exe", 0},
        {L"msedge.exe", L"C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\msedge.exe", 5},
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

        size_t pos = app.path.find_last_of(L"\\\\/");
        icon.processName = (pos != std::wstring::npos) ? app.path.substr(pos + 1) : app.path;

        if (app.path.find(L":") != std::wstring::npos) {
            icon.loadedIcon = m_iconLoader->LoadSystemIcon(app.path, ICON_SIZE);
        } else {
            wchar_t sysPath[MAX_PATH];
            GetSystemDirectoryW(sysPath, MAX_PATH);
            icon.loadedIcon = m_iconLoader->LoadFromExe(std::wstring(sysPath) + L"\\\\" + app.path, ICON_SIZE);
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
        if (!m_frameLimiter->ShouldSkipFrame()) OnPaint();
        m_frameLimiter->EndFrame();
    }
}

LRESULT CALLBACK DockWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_CREATE) {
        auto* self = static_cast<DockWindow*>(reinterpret_cast<LPCREATESTRUCT>(lParam)->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        return 0;
    }
    auto* self = reinterpret_cast<DockWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (self) return self->HandleMessage(msg, wParam, lParam);
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT DockWindow::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_SIZE: OnSize(LOWORD(lParam), HIWORD(lParam)); return 0;
        case WM_DPICHANGED: if (m_renderer) m_renderer->Resize(LOWORD(lParam), HIWORD(lParam)); return 0;
        case WM_KEYDOWN: if (wParam == VK_F1) CycleEffect(); return 0;
        case WM_MOUSEMOVE: {
            int mx = GET_X_LPARAM(lParam), my = GET_Y_LPARAM(lParam);
            int oldHover = m_hoveredIcon;
            m_hoveredIcon = -1;
            for (size_t i = 0; i < m_icons.size(); ++i) {
                float drawX = m_icons[i].baseX + (m_icons[i].width - m_icons[i].width * m_icons[i].scale) / 2.0f;
                float drawY = m_icons[i].baseY + m_icons[i].jumpOffsetY;
                float drawW = m_icons[i].width * m_icons[i].scale;
                float drawH = m_icons[i].height * m_icons[i].scale;
                if (mx >= drawX && mx < drawX + drawW && my >= drawY && my < drawY + drawH) {
                    m_hoveredIcon = static_cast<int>(i); break;
                }
            }
            if (oldHover != m_hoveredIcon) {
                if (oldHover >= 0) {
                    SetIconHover(oldHover, false);
                    if (m_previewIcon == oldHover) HideThumbnailPreview();
                }
                if (m_hoveredIcon >= 0) {
                    SetIconHover(m_hoveredIcon, true);
                    if (m_icons[m_hoveredIcon].type == IconType::App) ShowThumbnailPreview(m_hoveredIcon);
                }
            }
            return 0;
        }
        case WM_LBUTTONDOWN: {
            int mx = GET_X_LPARAM(lParam), my = GET_Y_LPARAM(lParam);
            for (size_t i = 0; i < m_icons.size(); ++i) {
                float drawX = m_icons[i].baseX + (m_icons[i].width - m_icons[i].width * m_icons[i].scale) / 2.0f;
                float drawY = m_icons[i].baseY + m_icons[i].jumpOffsetY;
                float drawW = m_icons[i].width * m_icons[i].scale;
                float drawH = m_icons[i].height * m_icons[i].scale;
                if (mx >= drawX && mx < drawX + drawW && my >= drawY && my < drawY + drawH) {
                    if (m_icons[i].type == IconType::StartButton) {
                        StartButton::OpenStartMenu(); TriggerJump(static_cast<int>(i));
                    } else if (m_icons[i].type == IconType::Tray) {
                        TrayIconManager::Instance().SendMouseEvent(m_icons[i].trayInfo, WM_LBUTTONUP);
                        TriggerJump(static_cast<int>(i));
                    } else {
                        LaunchOrActivate(static_cast<int>(i));
                    }
                    break;
                }
            }
            return 0;
        }
        case WM_RBUTTONDOWN: {
            int mx = GET_X_LPARAM(lParam), my = GET_Y_LPARAM(lParam);
            bool hit = false;
            for (size_t i = 0; i < m_icons.size(); ++i) {
                float drawX = m_icons[i].baseX + (m_icons[i].width - m_icons[i].width * m_icons[i].scale) / 2.0f;
                float drawY = m_icons[i].baseY + m_icons[i].jumpOffsetY;
                float drawW = m_icons[i].width * m_icons[i].scale;
                float drawH = m_icons[i].height * m_icons[i].scale;
                if (mx >= drawX && mx < drawX + drawW && my >= drawY && my < drawY + drawH) {
                    hit = true;
                    if (m_icons[i].type == IconType::Tray) {
                        TrayIconManager::Instance().SendMouseEvent(m_icons[i].trayInfo, WM_RBUTTONUP);
                    } else if (m_icons[i].type == IconType::App) {
                        TriggerBadgePulse(static_cast<int>(i));
                        POINT pt = { mx, my }; ClientToScreen(m_hwnd, &pt);
                        ShowContextMenu(static_cast<int>(i), pt.x, pt.y);
                    }
                    break;
                }
            }
            if (!hit) {
                POINT pt = { mx, my }; ClientToScreen(m_hwnd, &pt);
                ShowDockContextMenu(pt.x, pt.y);
            }
            return 0;
        }
        case WM_MBUTTONDOWN: {
            int mx = GET_X_LPARAM(lParam), my = GET_Y_LPARAM(lParam);
            for (size_t i = 0; i < m_icons.size(); ++i) {
                float drawX = m_icons[i].baseX + (m_icons[i].width - m_icons[i].width * m_icons[i].scale) / 2.0f;
                float drawY = m_icons[i].baseY + m_icons[i].jumpOffsetY;
                float drawW = m_icons[i].width * m_icons[i].scale;
                float drawH = m_icons[i].height * m_icons[i].scale;
                if (mx >= drawX && mx < drawX + drawW && my >= drawY && my < drawY + drawH) {
                    if (m_icons[i].type == IconType::App) {
                        auto windows = WindowManager::Instance().GetWindowsForProcess(m_icons[i].processName);
                        for (const auto& w : windows) WindowManager::Instance().CloseWindow(w.hwnd);
                    } else if (m_icons[i].type == IconType::Tray) {
                        TrayIconManager::Instance().SendMouseEvent(m_icons[i].trayInfo, WM_MBUTTONUP);
                    }
                    break;
                }
            }
            return 0;
        }
        case WM_LBUTTONDBLCLK: {
            int mx = GET_X_LPARAM(lParam), my = GET_Y_LPARAM(lParam);
            for (size_t i = 0; i < m_icons.size(); ++i) {
                float drawX = m_icons[i].baseX + (m_icons[i].width - m_icons[i].width * m_icons[i].scale) / 2.0f;
                float drawY = m_icons[i].baseY + m_icons[i].jumpOffsetY;
                float drawW = m_icons[i].width * m_icons[i].scale;
                float drawH = m_icons[i].height * m_icons[i].scale;
                if (mx >= drawX && mx < drawX + drawW && my >= drawY && my < drawY + drawH) {
                    if (m_icons[i].type == IconType::App) TriggerGenie(static_cast<int>(i));
                    break;
                }
            }
            return 0;
        }
        case WM_DESTROY: PostQuitMessage(0); return 0;
        default: return DefWindowProcW(m_hwnd, msg, wParam, lParam);
    }
}

void DockWindow::ShowDockContextMenu(int x, int y) {
    DockContextMenu::Show(m_hwnd, x, y);
}

void DockWindow::LaunchOrActivate(int iconIndex) {
    if (iconIndex < 0 || iconIndex >= static_cast<int>(m_icons.size())) return;
    auto& icon = m_icons[iconIndex];
    if (icon.isRunning) {
        auto windows = WindowManager::Instance().GetWindowsForProcess(icon.processName);
        if (!windows.empty()) {
            HWND fg = GetForegroundWindow();
            bool fgBelongs = false;
            for (const auto& w : windows) if (w.hwnd == fg) { fgBelongs = true; break; }
            if (fgBelongs && !IsIconic(fg)) WindowManager::Instance().MinimizeWindow(fg);
            else WindowManager::Instance().ActivateWindow(windows[0].hwnd);
        }
    } else {
        LaunchApp(icon);
    }
    TriggerJump(iconIndex);
}

void DockWindow::LaunchApp(const DockIcon& icon) {
    if (icon.appPath.find(L":") != std::wstring::npos) {
        ShellExecuteW(nullptr, L"open", icon.appPath.c_str(), nullptr, nullptr, SW_SHOW);
    } else if (!icon.exePath.empty()) {
        ShellExecuteW(nullptr, L"open", icon.exePath.c_str(), nullptr, nullptr, SW_SHOW);
    } else {
        ShellExecuteW(nullptr, L"open", icon.appPath.c_str(), nullptr, nullptr, SW_SHOW);
    }
    LOG_INFO("Launched: " + std::string(icon.appPath.begin(), icon.appPath.end()));
}

void DockWindow::ShowThumbnailPreview(int iconIndex) {
    if (!Config::Instance().Get().thumbnailPreviews) return;
    if (iconIndex < 0 || iconIndex >= static_cast<int>(m_icons.size())) return;
    auto& icon = m_icons[iconIndex];
    if (!icon.isRunning || icon.windowCount == 0) { HideThumbnailPreview(); return; }
    auto windows = WindowManager::Instance().GetWindowsForProcess(icon.processName);
    if (windows.empty()) { HideThumbnailPreview(); return; }
    
    HWND targetHwnd = windows[0].hwnd;
    RECT dockRc; GetWindowRect(m_hwnd, &dockRc);
    float iconCenterX = icon.baseX + icon.width / 2.0f;
    int previewWidth = 200, previewHeight = 150;
    int previewX = dockRc.left + static_cast<int>(iconCenterX) - previewWidth / 2;
    int previewY = dockRc.top - previewHeight - 8;
    RECT previewRc = { previewX, previewY, previewX + previewWidth, previewY + previewHeight };
    
    if (!m_thumbnailPreview) m_thumbnailPreview = std::make_unique<ThumbnailPreview>();
    m_thumbnailPreview->Show(targetHwnd, m_hwnd, previewRc);
    m_previewIcon = iconIndex;
}

void DockWindow::HideThumbnailPreview() {
    if (m_thumbnailPreview) m_thumbnailPreview->Hide();
    m_previewIcon = -1;
}

void DockWindow::ShowContextMenu(int iconIndex, int x, int y) {
    if (iconIndex < 0 || iconIndex >= static_cast<int>(m_icons.size())) return;
    auto& icon = m_icons[iconIndex];
    HMENU hMenu = CreatePopupMenu();
    
    auto jumpList = JumpListManager::Instance().GetJumpList(icon.exePath);
    if (!jumpList.empty()) {
        for (size_t i = 0; i < jumpList.size() && i < 10; ++i)
            AppendMenuW(hMenu, MF_STRING, static_cast<UINT>(1000 + i), jumpList[i].title.c_str());
        AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    }
    
    auto windows = WindowManager::Instance().GetWindowsForProcess(icon.processName);
    if (!windows.empty()) {
        AppendMenuW(hMenu, MF_STRING, (windows[0].isMinimized ? 100 : 101), windows[0].isMinimized ? L"Restore" : L"Minimize");
        AppendMenuW(hMenu, MF_STRING, 102, L"Close Window");
        if (windows.size() > 1) AppendMenuW(hMenu, MF_STRING, 103, (L"Close All (" + std::to_wstring(windows.size()) + L")").c_str());
        AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    }
    AppendMenuW(hMenu, MF_STRING, 200, icon.isPinned ? L"Unpin from Dock" : L"Pin to Dock");
    
    SetForegroundWindow(m_hwnd);
    int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, x, y, 0, m_hwnd, nullptr);
    DestroyMenu(hMenu);
    
    if (cmd >= 1000 && cmd < 1100) { size_t idx = cmd - 1000; if (idx < jumpList.size()) JumpListManager::Instance().LaunchItem(jumpList[idx]); }
    else if (cmd == 100 && !windows.empty()) WindowManager::Instance().RestoreWindow(windows[0].hwnd);
    else if (cmd == 101 && !windows.empty()) WindowManager::Instance().MinimizeWindow(windows[0].hwnd);
    else if (cmd == 102 && !windows.empty()) WindowManager::Instance().CloseWindow(windows[0].hwnd);
    else if (cmd == 103) for (const auto& w : windows) WindowManager::Instance().CloseWindow(w.hwnd);
    else if (cmd == 200) icon.isPinned = !icon.isPinned;
}

void DockWindow::CycleEffect() {
    m_currentEffect = (m_currentEffect + 1) % 3;
    const char* names[] = { "Solid", "Acrylic", "Liquid Glass" };
    LOG_INFO(std::string("Switched effect to: ") + names[m_currentEffect]);
}

void DockWindow::SetIconHover(int index, bool hovered) {
    if (index < 0 || index >= static_cast<int>(m_icons.size())) return;
    auto& cfg = Config::Instance().Get();
    float targetScale = hovered ? cfg.magnificationScale : 1.0f;
    float speed = 250.0f * cfg.animationSpeed;
    EasingType easing = EasingFromString(cfg.magnificationEasing);

    if (m_icons[index].tweenIdScale >= 0) TweenEngine::Instance().RemoveTween(m_icons[index].tweenIdScale);
    m_icons[index].tweenIdScale = TweenEngine::Instance().AddTween(
        &m_icons[index].scale, m_icons[index].scale, targetScale, speed, easing);

    int range = static_cast<int>(cfg.magnificationRange);
    for (int offset = 1; offset <= range; ++offset) {
        float neighborScale = hovered ? (1.0f + (cfg.magnificationScale - 1.0f) * (1.0f - offset / (range + 1.0f))) : 1.0f;
        int left = index - offset;
        if (left >= 0) {
            if (m_icons[left].tweenIdScale >= 0) TweenEngine::Instance().RemoveTween(m_icons[left].tweenIdScale);
            m_icons[left].tweenIdScale = TweenEngine::Instance().AddTween(
                &m_icons[left].scale, m_icons[left].scale, neighborScale, speed, easing);
        }
        int right = index + offset;
        if (right < static_cast<int>(m_icons.size())) {
            if (m_icons[right].tweenIdScale >= 0) TweenEngine::Instance().RemoveTween(m_icons[right].tweenIdScale);
            m_icons[right].tweenIdScale = TweenEngine::Instance().AddTween(
                &m_icons[right].scale, m_icons[right].scale, neighborScale, speed, easing);
        }
    }
}

void DockWindow::TriggerJump(int index) {
    if (index < 0 || index >= static_cast<int>(m_icons.size())) return;
    auto& cfg = Config::Instance().Get();
    if (!cfg.jumpAnimation) return;
    auto& icon = m_icons[index];
    float speed = cfg.jumpAnimationSpeed * cfg.animationSpeed;
    if (icon.tweenIdJump >= 0) TweenEngine::Instance().RemoveTween(icon.tweenIdJump);
    icon.tweenIdJump = TweenEngine::Instance().AddTween(&icon.jumpOffsetY, 0.0f, -20.0f, speed * 0.4f, EasingType::EaseOutBack,
        [this, index, speed]() {
            if (index < static_cast<int>(m_icons.size())) {
                m_icons[index].tweenIdJump = TweenEngine::Instance().AddTween(
                    &m_icons[index].jumpOffsetY, -20.0f, 0.0f, speed * 0.6f, EasingType::EaseOutBounce);
            }
        });
    TweenEngine::Instance().AddTween(&icon.jumpScale, 1.0f, 1.15f, speed * 0.3f, EasingType::EaseOutBack,
        [this, index, speed]() {
            if (index < static_cast<int>(m_icons.size())) {
                TweenEngine::Instance().AddTween(&m_icons[index].jumpScale, 1.15f, 1.0f, speed * 0.3f, EasingType::EaseOut);
            }
        });
}

void DockWindow::TriggerGenie(int index) {
    if (index < 0 || index >= static_cast<int>(m_icons.size())) return;
    auto& cfg = Config::Instance().Get();
    if (!cfg.genieEffect) return;
    auto& icon = m_icons[index];
    float speed = cfg.genieAnimationSpeed * cfg.animationSpeed;
    if (icon.tweenIdGenie >= 0) TweenEngine::Instance().RemoveTween(icon.tweenIdGenie);
    icon.tweenIdGenie = TweenEngine::Instance().AddTween(&icon.genieSkew, 0.0f, -0.3f, speed * 0.5f, EasingType::EaseInOut,
        [this, index, speed]() {
            if (index < static_cast<int>(m_icons.size())) {
                TweenEngine::Instance().AddTween(&m_icons[index].genieSkew, -0.3f, 0.0f, speed * 0.5f, EasingType::EaseOut);
            }
        });
    TweenEngine::Instance().AddTween(&icon.genieScaleY, 1.0f, 0.1f, speed, EasingType::EaseInOut,
        [this, index, speed]() {
            if (index < static_cast<int>(m_icons.size())) {
                TweenEngine::Instance().AddTween(&m_icons[index].genieScaleY, 0.1f, 1.0f, speed * 0.8f, EasingType::EaseOutBack);
            }
        });
    TweenEngine::Instance().AddTween(&icon.genieOpacity, 1.0f, 0.3f, speed * 0.5f, EasingType::EaseInOut,
        [this, index, speed]() {
            if (index < static_cast<int>(m_icons.size())) {
                TweenEngine::Instance().AddTween(&m_icons[index].genieOpacity, 0.3f, 1.0f, speed * 0.5f, EasingType::EaseOut);
            }
        });
}

void DockWindow::TriggerBadgePulse(int index) {
    if (index < 0 || index >= static_cast<int>(m_icons.size())) return;
    auto& cfg = Config::Instance().Get();
    if (!cfg.badgePulse) return;
    auto& icon = m_icons[index];
    float speed = cfg.badgePulseSpeed * cfg.animationSpeed;
    if (icon.tweenIdBadge >= 0) TweenEngine::Instance().RemoveTween(icon.tweenIdBadge);
    icon.tweenIdBadge = TweenEngine::Instance().AddTween(&icon.badgeScale, 1.0f, 1.5f, speed * 0.5f, EasingType::EaseOutBack,
        [this, index, speed]() {
            if (index < static_cast<int>(m_icons.size())) {
                m_icons[index].tweenIdBadge = TweenEngine::Instance().AddTween(
                    &m_icons[index].badgeScale, 1.5f, 1.0f, speed * 0.5f, EasingType::EaseOut);
            }
        });
}

void DockWindow::UpdateAnimations(float deltaTime) {
    TweenEngine::Instance().Update(deltaTime);
    auto& cfg = Config::Instance().Get();
    if (cfg.runningIndicatorPulse) {
        float pulse = 0.7f + 0.3f * std::sin(m_animTime * 3.0f);
        for (auto& icon : m_icons) {
            if (icon.isRunning || (icon.isPinned && icon.type == IconType::App)) icon.indicatorOpacity = pulse;
        }
    }
    for (auto& icon : m_icons) {
        if (icon.loadedIcon.isAnimated) icon.animator.Update();
    }
    static float stateTimer = 0.0f;
    stateTimer += deltaTime;
    if (stateTimer > 1.0f) { stateTimer = 0.0f; UpdateRunningStates(); }
}

void DockWindow::OnPaint() {
    if (!m_renderer || !m_renderer->IsInitialized()) return;
    m_renderer->BeginDraw();
    m_renderer->Clear(0.0f, 0.0f, 0.0f, 0.0f);
    DrawBackgroundEffect();
    DrawIcons();
    
    // Render widgets after tray icons
    float widgetX = 0;
    for (const auto& icon : m_icons) {
        if (icon.type == IconType::Tray) {
            widgetX = std::max(widgetX, icon.baseX + icon.width * icon.scale + 12);
        }
    }
    if (widgetX > 0) {
        PluginManager::Instance().Render(m_renderer->GetRenderTarget(), m_renderer->GetWriteFactory(),
            widgetX, m_slideOffsetY + 8, DOCK_HEIGHT - 16);
    }
    
    if (Config::Instance().Get().showFPS) DrawFPS();
    m_renderer->EndDraw();
}

void DockWindow::DrawBackgroundEffect() {
    if (!m_effectRenderer || !m_effectRenderer->IsInitialized()) {
        RECT rc; GetClientRect(m_hwnd, &rc);
        float w = static_cast<float>(rc.right - rc.left);
        float h = static_cast<float>(rc.bottom - rc.top);
        auto& colors = ThemeManager::Instance().GetColors();
        m_renderer->DrawRoundedRect(0, m_slideOffsetY, w, h, DOCK_CORNER_RADIUS, colors.background.r, colors.background.g, colors.background.b, 0.88f * m_slideOpacity);
        return;
    }

    RECT rc; GetClientRect(m_hwnd, &rc);
    int w = rc.right - rc.left, h = rc.bottom - rc.top;

    Microsoft::WRL::ComPtr<ID2D1Bitmap> screenBitmap;
    POINT pt; GetWindowRect(m_hwnd, reinterpret_cast<LPRECT>(&pt));
    bool hasScreen = ScreenCapture::CaptureRegion(pt.x, pt.y, w, h, m_effectRenderer->GetContext(), screenBitmap.GetAddressOf());

    m_effectRenderer->BeginDraw();
    m_effectRenderer->GetContext()->Clear(D2D1::ColorF(0.1f, 0.1f, 0.15f, 1.0f));
    if (hasScreen && screenBitmap) m_effectRenderer->GetContext()->DrawBitmap(screenBitmap.Get());
    m_effectRenderer->EndDraw();

    ID2D1Image* effectOutput = nullptr;
    auto& cfg = Config::Instance().Get();
    if (m_currentEffect == 1 && m_acrylic) {
        m_acrylic->SetBlurRadius(cfg.blurRadius);
        m_acrylic->SetTint(cfg.tintR, cfg.tintG, cfg.tintB, cfg.tintOpacity);
        effectOutput = m_acrylic->Apply(m_effectRenderer->GetTargetBitmap());
    } else if (m_currentEffect == 2 && m_liquidGlass) {
        m_liquidGlass->SetRefractionStrength(cfg.refractionStrength);
        m_liquidGlass->SetGlowIntensity(cfg.glowIntensity);
        m_liquidGlass->SetSaturation(cfg.liquidGlassSaturation);
        effectOutput = m_liquidGlass->Apply(m_effectRenderer->GetTargetBitmap());
    }

    float drawY = m_slideOffsetY;
    if (effectOutput) {
        m_renderer->DrawBitmap(reinterpret_cast<ID2D1Bitmap*>(effectOutput), 0, drawY, static_cast<float>(w), static_cast<float>(h), 0.9f * m_slideOpacity);
    } else {
        m_renderer->DrawBitmap(m_effectRenderer->GetTargetBitmap(), 0, drawY, static_cast<float>(w), static_cast<float>(h), 0.9f * m_slideOpacity);
    }

    m_renderer->FillRect(DOCK_CORNER_RADIUS, drawY, w - DOCK_CORNER_RADIUS * 2, 1.0f, 0.5f, 0.5f, 0.55f, 0.4f * m_slideOpacity);
}

void DockWindow::DrawIcons() {
    ID2D1Bitmap* atlas = m_textureAtlas->GetAtlasBitmap();
    float slideY = m_slideOffsetY;
    for (size_t i = 0; i < m_icons.size(); ++i) {
        auto& icon = m_icons[i];
        float combinedScale = icon.scale * icon.jumpScale * icon.genieScaleY;
        float scaledW = icon.width * combinedScale;
        float scaledH = icon.height * combinedScale;
        float drawX = icon.baseX + (icon.width - scaledW) / 2.0f;
        float drawY = icon.baseY + icon.jumpOffsetY + slideY + (icon.height - scaledH) / 2.0f;
        float opacity = icon.jumpOpacity * icon.genieOpacity * m_slideOpacity;
        if (opacity <= 0.01f) continue;

        float skewOffset = icon.genieSkew * (drawY - icon.baseY - slideY);
        m_renderer->DrawRoundedRect(drawX + 2 + skewOffset, drawY + 2, scaledW, scaledH, 10.0f, 0.0f, 0.0f, 0.0f, 0.2f * opacity);

        if (atlas && icon.atlasEntry.width > 0) {
            m_renderer->DrawBitmapFromAtlas(atlas, drawX + skewOffset, drawY, scaledW, scaledH,
                icon.atlasEntry.u0, icon.atlasEntry.v0, icon.atlasEntry.u1, icon.atlasEntry.v1, opacity);
        } else if (!icon.loadedIcon.frames.empty()) {
            ID2D1Bitmap* frame = icon.animator.GetCurrentFrame();
            if (frame) m_renderer->DrawBitmap(frame, drawX + skewOffset, drawY, scaledW, scaledH, opacity);
        } else {
            m_renderer->DrawRoundedRect(drawX + skewOffset, drawY, scaledW, scaledH, 10.0f, 0.3f, 0.3f, 0.35f, 0.5f * opacity);
        }

        if (icon.type == IconType::App && (icon.isRunning || icon.isPinned)) {
            float dotSize = 4.0f * icon.scale;
            float dotX = drawX + scaledW / 2.0f - dotSize / 2.0f;
            float dotY = drawY + scaledH + 3.0f;
            float alpha = (icon.isRunning ? 1.0f : 0.3f) * icon.indicatorOpacity * m_slideOpacity;
            m_renderer->FillRect(dotX, dotY, dotSize, dotSize, 1.0f, 1.0f, 1.0f, alpha);
        }

        if (icon.type == IconType::App && icon.badgeCount > 0) {
            float badgeBaseX = drawX + scaledW - 8.0f;
            float badgeBaseY = drawY - 2.0f;
            float badgeSize = 16.0f * icon.badgeScale;
            float badgeX = badgeBaseX - badgeSize / 2.0f;
            float badgeY = badgeBaseY - badgeSize / 2.0f;
            m_renderer->DrawRoundedRect(badgeX, badgeY, badgeSize, badgeSize, badgeSize / 2.0f, 0.95f, 0.2f, 0.2f, 1.0f * opacity);
            m_renderer->DrawRoundedRect(badgeX, badgeY, badgeSize, badgeSize, badgeSize / 2.0f, 1.0f, 1.0f, 1.0f, 0.8f * opacity);
            m_badgeRenderer->DrawBadge(m_renderer->GetRenderTarget(), drawX, drawY, icon.badgeCount, scaledW);
        }
    }
}

void DockWindow::DrawFPS() {
    if (!m_frameLimiter) return;
    float fps = m_frameLimiter->GetCurrentFPS();
    std::wstring text = L"FPS: " + std::to_wstring(static_cast<int>(fps));
    auto& colors = ThemeManager::Instance().GetColors();
    m_renderer->DrawTextLayout(text, 10, 5 + m_slideOffsetY, colors.text.r, colors.text.g, colors.text.b, 10.0f);
}

void DockWindow::OnSize(UINT width, UINT height) {
    if (m_renderer) m_renderer->Resize(width, height);
    if (m_effectRenderer && m_effectRenderer->IsInitialized()) {
        m_effectRenderer->Resize(width, height);
        if (m_acrylic) m_acrylic->Initialize(m_effectRenderer->GetContext(), width, height);
        if (m_liquidGlass) m_liquidGlass->Initialize(m_effectRenderer->GetContext(), width, height);
    }
    UpdateIconPositions();
}

void DockWindow::UpdateWindowPosition() {
    if (!m_hwnd) return;
    RECT workArea;
    if (m_monitor) {
        workArea = m_monitor->workArea;
    } else {
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);
    }
    int screenWidth = workArea.right - workArea.left;
    int dockWidth = screenWidth - (DOCK_MARGIN * 2);
    int x = workArea.left + DOCK_MARGIN;
    int y = workArea.bottom - DOCK_HEIGHT - DOCK_MARGIN;
    SetWindowPos(m_hwnd, nullptr, x, y, dockWidth, DOCK_HEIGHT, SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
}

void DockWindow::Update(float deltaTime) {
    m_animTime += deltaTime;
    UpdateAnimations(deltaTime);
    if (m_liquidGlass) m_liquidGlass->UpdateAnimation(deltaTime);
}

void DockWindow::Render() {
    if (!m_frameLimiter) return;
    m_frameLimiter->BeginFrame();
    if (!m_frameLimiter->ShouldSkipFrame()) OnPaint();
    m_frameLimiter->EndFrame();
}