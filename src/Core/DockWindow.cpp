#include "DockWindow.h"
#include "../Utils/Logger.h"
#include "../Utils/ScreenCapture.h"
#include <windowsx.h>
#include <chrono>
#include <cmath>

DockWindow::DockWindow() {}
DockWindow::~DockWindow() { Destroy(); }

void DockWindow::Destroy() { if (m_hwnd) { DestroyWindow(m_hwnd); m_hwnd = nullptr; } }

bool DockWindow::Create(HINSTANCE hInstance) {
    m_hInstance = hInstance;
    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc = DockWindow::WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"DockForgeDockWindow";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    if (!RegisterClassExW(&wc)) { LOG_ERROR("Failed to register window class"); return false; }

    RECT workArea; SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);
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
    UpdateIconPositions();

    LOG_INFO("Dock window created with icon support");
    return true;
}

void DockWindow::LoadDemoIcons() {
    // Load system icons as demo placeholders
    // In production, these would come from running apps / pinned apps config
    std::vector<std::pair<std::wstring, int>> demoApps = {
        {L"explorer.exe", 3},
        {L"notepad.exe", 0},
        {L"calc.exe", 0},
        {L"msedge.exe", 5},
        {L"ms-settings:", 0},
    };

    for (auto& [path, badge] : demoApps) {
        DockIcon icon;
        icon.appPath = path;
        icon.isPinned = true;
        icon.badgeCount = badge;

        // Try to load real icon
        if (path.find(L":") != std::wstring::npos) {
            icon.loadedIcon = m_iconLoader->LoadSystemIcon(path, ICON_SIZE);
        } else {
            wchar_t sysPath[MAX_PATH];
            GetSystemDirectoryW(sysPath, MAX_PATH);
            std::wstring fullPath = std::wstring(sysPath) + L"\\" + path;
            icon.loadedIcon = m_iconLoader->LoadFromExe(fullPath, ICON_SIZE);
        }

        if (!icon.loadedIcon.frames.empty()) {
            icon.animator.SetIcon(icon.loadedIcon);
            // Add first frame to atlas
            AtlasEntry entry;
            if (m_textureAtlas->AddBitmap(icon.loadedIcon.frames[0].Get(), entry)) {
                icon.atlasEntry = entry;
            }
        }

        m_icons.push_back(std::move(icon));
    }

    LOG_INFO("Loaded " + std::to_string(m_icons.size()) + " demo icons");
}

void DockWindow::UpdateIconPositions() {
    RECT rc; GetClientRect(m_hwnd, &rc);
    float width = static_cast<float>(rc.right - rc.left);
    float height = static_cast<float>(rc.bottom - rc.top);

    float totalIconWidth = m_icons.size() * ICON_SIZE + (m_icons.size() - 1) * ICON_PADDING;
    float startX = (width - totalIconWidth) / 2.0f;
    float iconY = (height - ICON_SIZE) / 2.0f;

    for (size_t i = 0; i < m_icons.size(); ++i) {
        m_icons[i].x = startX + i * (ICON_SIZE + ICON_PADDING);
        m_icons[i].y = iconY;
        m_icons[i].width = ICON_SIZE;
        m_icons[i].height = ICON_SIZE;
    }
}

void DockWindow::Show() { if (m_hwnd) { ShowWindow(m_hwnd, SW_SHOW); UpdateWindow(m_hwnd); } }
void DockWindow::Hide() { if (m_hwnd) ShowWindow(m_hwnd, SW_HIDE); }

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
            int mx = GET_X_LPARAM(lParam);
            int my = GET_Y_LPARAM(lParam);
            m_hoveredIcon = -1;
            for (size_t i = 0; i < m_icons.size(); ++i) {
                if (mx >= m_icons[i].x && mx < m_icons[i].x + m_icons[i].width &&
                    my >= m_icons[i].y && my < m_icons[i].y + m_icons[i].height) {
                    m_hoveredIcon = static_cast<int>(i);
                    break;
                }
            }
            return 0;
        }
        case WM_DESTROY: PostQuitMessage(0); return 0;
        default: return DefWindowProcW(m_hwnd, msg, wParam, lParam);
    }
}

void DockWindow::CycleEffect() {
    m_currentEffect = (m_currentEffect + 1) % 3;
    const char* names[] = { "Solid", "Acrylic", "Liquid Glass" };
    LOG_INFO(std::string("Switched effect to: ") + names[m_currentEffect]);
}

void DockWindow::UpdateAnimations(float deltaTime) {
    // Magnification effect on hover
    for (size_t i = 0; i < m_icons.size(); ++i) {
        float target = (static_cast<int>(i) == m_hoveredIcon) ? 1.3f : 1.0f;
        float speed = 10.0f;
        m_icons[i].currentScale += (target - m_icons[i].currentScale) * speed * deltaTime;
        if (std::abs(m_icons[i].currentScale - target) < 0.01f) m_icons[i].currentScale = target;
    }

    // Update animated icons
    for (auto& icon : m_icons) {
        if (icon.loadedIcon.isAnimated) {
            icon.animator.Update();
        }
    }
}

void DockWindow::OnPaint() {
    if (!m_renderer || !m_renderer->IsInitialized()) return;
    m_renderer->BeginDraw();
    m_renderer->Clear(0.0f, 0.0f, 0.0f, 0.0f);

    DrawBackgroundEffect();
    DrawIcons();
    if (Config::Instance().Get().showFPS) DrawFPS();

    m_renderer->EndDraw();
}

void DockWindow::DrawBackgroundEffect() {
    if (!m_effectRenderer || !m_effectRenderer->IsInitialized()) {
        RECT rc; GetClientRect(m_hwnd, &rc);
        float width = static_cast<float>(rc.right - rc.left);
        float height = static_cast<float>(rc.bottom - rc.top);
        m_renderer->DrawRoundedRect(0, 0, width, height, DOCK_CORNER_RADIUS, 0.12f, 0.12f, 0.14f, 0.88f);
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

    if (effectOutput) {
        m_renderer->DrawBitmap(reinterpret_cast<ID2D1Bitmap*>(effectOutput), 0, 0, static_cast<float>(w), static_cast<float>(h), 0.9f);
    } else {
        m_renderer->DrawBitmap(m_effectRenderer->GetTargetBitmap(), 0, 0, static_cast<float>(w), static_cast<float>(h), 0.9f);
    }

    m_renderer->FillRect(DOCK_CORNER_RADIUS, 0, w - DOCK_CORNER_RADIUS * 2, 1.0f, 0.5f, 0.5f, 0.55f, 0.4f);
}

void DockWindow::DrawIcons() {
    ID2D1Bitmap* atlas = m_textureAtlas->GetAtlasBitmap();

    for (size_t i = 0; i < m_icons.size(); ++i) {
        auto& icon = m_icons[i];
        float scale = icon.currentScale;
        float scaledW = icon.width * scale;
        float scaledH = icon.height * scale;
        float drawX = icon.x + (icon.width - scaledW) / 2.0f;
        float drawY = icon.y + (icon.height - scaledH) / 2.0f;

        // Draw icon shadow/glow
        m_renderer->DrawRoundedRect(drawX + 2, drawY + 2, scaledW, scaledH, 10.0f, 0.0f, 0.0f, 0.0f, 0.3f);

        // Draw icon from atlas or direct bitmap
        if (atlas && icon.atlasEntry.width > 0) {
            m_renderer->DrawBitmapFromAtlas(atlas, drawX, drawY, scaledW, scaledH,
                icon.atlasEntry.u0, icon.atlasEntry.v0, icon.atlasEntry.u1, icon.atlasEntry.v1, 1.0f);
        } else if (!icon.loadedIcon.frames.empty()) {
            ID2D1Bitmap* frame = icon.animator.GetCurrentFrame();
            if (frame) m_renderer->DrawBitmap(frame, drawX, drawY, scaledW, scaledH, 1.0f);
        } else {
            // Fallback placeholder
            m_renderer->DrawRoundedRect(drawX, drawY, scaledW, scaledH, 10.0f, 0.3f, 0.3f, 0.35f, 0.5f);
        }

        // Running indicator (dot)
        if (icon.isRunning || icon.isPinned) {
            float dotSize = 4.0f;
            float dotX = drawX + scaledW / 2.0f - dotSize / 2.0f;
            float dotY = drawY + scaledH + 3.0f;
            float alpha = icon.isRunning ? 1.0f : 0.3f;
            m_renderer->FillRect(dotX, dotY, dotSize, dotSize, 1.0f, 1.0f, 1.0f, alpha);
        }

        // Badge
        if (icon.badgeCount > 0) {
            m_badgeRenderer->DrawBadge(m_renderer->GetRenderTarget(), drawX, drawY, icon.badgeCount, scaledW);
        }
    }
}

void DockWindow::DrawFPS() {
    if (!m_frameLimiter) return;
    float fps = m_frameLimiter->GetCurrentFPS();
    std::wstring text = L"FPS: " + std::to_wstring(static_cast<int>(fps));
    m_renderer->DrawTextLayout(text, 10, 5, 0.0f, 1.0f, 0.0f, 10.0f);
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
    RECT workArea; SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);
    int screenWidth = workArea.right - workArea.left;
    int dockWidth = screenWidth - (DOCK_MARGIN * 2);
    int x = workArea.left + DOCK_MARGIN;
    int y = workArea.bottom - DOCK_HEIGHT - DOCK_MARGIN;
    SetWindowPos(m_hwnd, nullptr, x, y, dockWidth, DOCK_HEIGHT, SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
}
