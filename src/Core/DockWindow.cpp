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
    StartSlideIn();

    LOG_INFO("Dock window created with TweenEngine");
    return true;
}

void DockWindow::LoadDemoIcons() {
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
        icon.scale = 1.0f;
        icon.jumpOffsetY = 0.0f;
        icon.jumpScale = 1.0f;
        icon.jumpOpacity = 1.0f;
        icon.genieSkew = 0.0f;
        icon.genieScaleY = 1.0f;
        icon.genieOpacity = 1.0f;
        icon.badgeScale = 1.0f;
        icon.indicatorOpacity = 1.0f;

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
        m_icons[i].baseX = startX + i * (ICON_SIZE + ICON_PADDING);
        m_icons[i].baseY = iconY;
        m_icons[i].width = ICON_SIZE;
        m_icons[i].height = ICON_SIZE;
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
            int oldHover = m_hoveredIcon;
            m_hoveredIcon = -1;
            for (size_t i = 0; i < m_icons.size(); ++i) {
                float drawX = m_icons[i].baseX + (m_icons[i].width - m_icons[i].width * m_icons[i].scale) / 2.0f;
                float drawY = m_icons[i].baseY + m_icons[i].jumpOffsetY;
                float drawW = m_icons[i].width * m_icons[i].scale;
                float drawH = m_icons[i].height * m_icons[i].scale;
                if (mx >= drawX && mx < drawX + drawW && my >= drawY && my < drawY + drawH) {
                    m_hoveredIcon = static_cast<int>(i);
                    break;
                }
            }
            if (oldHover != m_hoveredIcon) {
                if (oldHover >= 0) SetIconHover(oldHover, false);
                if (m_hoveredIcon >= 0) SetIconHover(m_hoveredIcon, true);
            }
            return 0;
        }
        case WM_LBUTTONDOWN: {
            int mx = GET_X_LPARAM(lParam);
            int my = GET_Y_LPARAM(lParam);
            for (size_t i = 0; i < m_icons.size(); ++i) {
                float drawX = m_icons[i].baseX + (m_icons[i].width - m_icons[i].width * m_icons[i].scale) / 2.0f;
                float drawY = m_icons[i].baseY + m_icons[i].jumpOffsetY;
                float drawW = m_icons[i].width * m_icons[i].scale;
                float drawH = m_icons[i].height * m_icons[i].scale;
                if (mx >= drawX && mx < drawX + drawW && my >= drawY && my < drawY + drawH) {
                    TriggerJump(static_cast<int>(i));
                    break;
                }
            }
            return 0;
        }
        case WM_RBUTTONDOWN: {
            int mx = GET_X_LPARAM(lParam);
            int my = GET_Y_LPARAM(lParam);
            for (size_t i = 0; i < m_icons.size(); ++i) {
                float drawX = m_icons[i].baseX + (m_icons[i].width - m_icons[i].width * m_icons[i].scale) / 2.0f;
                float drawY = m_icons[i].baseY + m_icons[i].jumpOffsetY;
                float drawW = m_icons[i].width * m_icons[i].scale;
                float drawH = m_icons[i].height * m_icons[i].scale;
                if (mx >= drawX && mx < drawX + drawW && my >= drawY && my < drawY + drawH) {
                    TriggerBadgePulse(static_cast<int>(i));
                    break;
                }
            }
            return 0;
        }
        case WM_LBUTTONDBLCLK: {
            int mx = GET_X_LPARAM(lParam);
            int my = GET_Y_LPARAM(lParam);
            for (size_t i = 0; i < m_icons.size(); ++i) {
                float drawX = m_icons[i].baseX + (m_icons[i].width - m_icons[i].width * m_icons[i].scale) / 2.0f;
                float drawY = m_icons[i].baseY + m_icons[i].jumpOffsetY;
                float drawW = m_icons[i].width * m_icons[i].scale;
                float drawH = m_icons[i].height * m_icons[i].scale;
                if (mx >= drawX && mx < drawX + drawW && my >= drawY && my < drawY + drawH) {
                    TriggerGenie(static_cast<int>(i));
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

void DockWindow::SetIconHover(int index, bool hovered) {
    if (index < 0 || index >= static_cast<int>(m_icons.size())) return;
    auto& cfg = Config::Instance().Get();
    float targetScale = hovered ? cfg.magnificationScale : 1.0f;
    float speed = 250.0f * cfg.animationSpeed;
    EasingType easing = EasingFromString(cfg.magnificationEasing);

    // Remove existing scale tween
    if (m_icons[index].tweenIdScale >= 0) {
        TweenEngine::Instance().RemoveTween(m_icons[index].tweenIdScale);
    }
    m_icons[index].tweenIdScale = TweenEngine::Instance().AddTween(
        &m_icons[index].scale, m_icons[index].scale, targetScale, speed, easing);

    // Magnification range: affect neighbors
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

    // Remove existing jump tweens
    if (icon.tweenIdJump >= 0) TweenEngine::Instance().RemoveTween(icon.tweenIdJump);

    // Phase 1: jump up
    icon.tweenIdJump = TweenEngine::Instance().AddTween(&icon.jumpOffsetY, 0.0f, -20.0f, speed * 0.4f, EasingType::EaseOutBack,
        [this, index, speed]() {
            // Phase 2: fall down
            if (index < static_cast<int>(m_icons.size())) {
                m_icons[index].tweenIdJump = TweenEngine::Instance().AddTween(
                    &m_icons[index].jumpOffsetY, -20.0f, 0.0f, speed * 0.6f, EasingType::EaseOutBounce);
            }
        });

    // Scale pulse
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

    // Pseudo-genie: skew + scaleY + opacity
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

    // Running indicator pulse
    auto& cfg = Config::Instance().Get();
    if (cfg.runningIndicatorPulse) {
        float pulse = 0.7f + 0.3f * std::sin(m_animTime * 3.0f);
        for (auto& icon : m_icons) {
            if (icon.isRunning || icon.isPinned) {
                icon.indicatorOpacity = pulse;
            }
        }
    }

    // Update animated icons
    for (auto& icon : m_icons) {
        if (icon.loadedIcon.isAnimated) icon.animator.Update();
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
        float w = static_cast<float>(rc.right - rc.left);
        float h = static_cast<float>(rc.bottom - rc.top);
        m_renderer->DrawRoundedRect(0, m_slideOffsetY, w, h, DOCK_CORNER_RADIUS, 0.12f, 0.12f, 0.14f, 0.88f * m_slideOpacity);
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

        // Apply genie skew via transform (simplified: offset based on Y)
        float skewOffset = icon.genieSkew * (drawY - icon.baseY - slideY);

        // Shadow
        m_renderer->DrawRoundedRect(drawX + 2 + skewOffset, drawY + 2, scaledW, scaledH, 10.0f, 0.0f, 0.0f, 0.0f, 0.2f * opacity);

        // Icon
        if (atlas && icon.atlasEntry.width > 0) {
            m_renderer->DrawBitmapFromAtlas(atlas, drawX + skewOffset, drawY, scaledW, scaledH,
                icon.atlasEntry.u0, icon.atlasEntry.v0, icon.atlasEntry.u1, icon.atlasEntry.v1, opacity);
        } else if (!icon.loadedIcon.frames.empty()) {
            ID2D1Bitmap* frame = icon.animator.GetCurrentFrame();
            if (frame) m_renderer->DrawBitmap(frame, drawX + skewOffset, drawY, scaledW, scaledH, opacity);
        } else {
            m_renderer->DrawRoundedRect(drawX + skewOffset, drawY, scaledW, scaledH, 10.0f, 0.3f, 0.3f, 0.35f, 0.5f * opacity);
        }

        // Running indicator
        if (icon.isRunning || icon.isPinned) {
            float dotSize = 4.0f * icon.scale;
            float dotX = drawX + scaledW / 2.0f - dotSize / 2.0f;
            float dotY = drawY + scaledH + 3.0f;
            float alpha = (icon.isRunning ? 1.0f : 0.3f) * icon.indicatorOpacity * m_slideOpacity;
            m_renderer->FillRect(dotX, dotY, dotSize, dotSize, 1.0f, 1.0f, 1.0f, alpha);
        }

        // Badge
        if (icon.badgeCount > 0) {
            float badgeBaseX = drawX + scaledW - 8.0f;
            float badgeBaseY = drawY - 2.0f;
            float badgeSize = 16.0f * icon.badgeScale;
            float badgeX = badgeBaseX - badgeSize / 2.0f;
            float badgeY = badgeBaseY - badgeSize / 2.0f;

            // Draw badge background manually (simplified without text for now)
            m_renderer->DrawRoundedRect(badgeX, badgeY, badgeSize, badgeSize, badgeSize / 2.0f,
                0.95f, 0.2f, 0.2f, 1.0f * opacity);
            m_renderer->DrawRoundedRect(badgeX, badgeY, badgeSize, badgeSize, badgeSize / 2.0f,
                1.0f, 1.0f, 1.0f, 0.8f * opacity); // border

            // Use BadgeRenderer for text
            m_badgeRenderer->DrawBadge(m_renderer->GetRenderTarget(), drawX, drawY, icon.badgeCount, scaledW);
        }
    }
}

void DockWindow::DrawFPS() {
    if (!m_frameLimiter) return;
    float fps = m_frameLimiter->GetCurrentFPS();
    std::wstring text = L"FPS: " + std::to_wstring(static_cast<int>(fps));
    m_renderer->DrawTextLayout(text, 10, 5 + m_slideOffsetY, 0.0f, 1.0f, 0.0f, 10.0f);
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
