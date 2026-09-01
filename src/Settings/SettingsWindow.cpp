#include "SettingsWindow.h"
#include "../Utils/Config.h"
#include "../Utils/Theme.h"
#include "../Utils/PerformanceProfile.h"
#include "../Utils/Logger.h"
#include <windowsx.h>
#include <cmath>

SettingsWindow& SettingsWindow::Instance() {
    static SettingsWindow instance;
    return instance;
}

bool SettingsWindow::Create(HINSTANCE hInstance) {
    m_hInstance = hInstance;
    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc = SettingsWindow::WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"DockForgeSettings";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    RegisterClassExW(&wc);

    RECT workArea; SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);
    int cx = workArea.left + ((workArea.right - workArea.left) - WIN_WIDTH) / 2;
    int cy = workArea.top + ((workArea.bottom - workArea.top) - WIN_HEIGHT) / 2;

    m_hwnd = CreateWindowExW(WS_EX_DLGMODALFRAME | WS_EX_TOPMOST, L"DockForgeSettings", L"DockForge Settings",
        WS_POPUP | WS_CAPTION | WS_SYSMENU, cx, cy, WIN_WIDTH, WIN_HEIGHT, nullptr, nullptr, hInstance, this);
    if (!m_hwnd) return false;

    m_renderer = std::make_unique<D2DRenderer>();
    if (!m_renderer->Initialize(m_hwnd)) return false;

    BuildLayout();
    SyncValuesFromConfig();
    return true;
}

void SettingsWindow::BuildLayout() {
    m_controls.clear();
    float y = PADDING;
    float x = TAB_WIDTH + PADDING;
    float w = WIN_WIDTH - TAB_WIDTH - PADDING * 2;
    float h = 32;

    auto add = [&](int cat, const std::string& id, const std::string& label, ControlType type) -> SettingControl& {
        SettingControl c; c.category = cat; c.id = id; c.label = label; c.type = type;
        c.x = x; c.y = y; c.w = w; c.h = h;
        m_controls.push_back(c);
        y += ROW_H;
        return m_controls.back();
    };

    // General
    add(0, "showStartButton", "Show Start Button", ControlType::Toggle).boolValue = Config::Instance().Get().showStartButton;
    add(0, "showTrayIcons", "Show Tray Icons", ControlType::Toggle).boolValue = Config::Instance().Get().showTrayIcons;
    add(0, "showFPS", "Show FPS Counter", ControlType::Toggle).boolValue = Config::Instance().Get().showFPS;
    add(0, "sep1", "", ControlType::Separator);
    add(0, "jumpLists", "Enable Jump Lists", ControlType::Toggle).boolValue = Config::Instance().Get().jumpLists;
    add(0, "thumbnailPreviews", "Thumbnail Previews", ControlType::Toggle).boolValue = Config::Instance().Get().thumbnailPreviews;

    y = PADDING;
    // Appearance
    auto& theme = add(1, "theme", "Theme", ControlType::Dropdown);
    theme.options = { "Auto", "Light", "Dark" };
    theme.selectedIndex = (ThemeManager::Instance().GetMode() == ThemeMode::Light) ? 1 : (ThemeManager::Instance().GetMode() == ThemeMode::Dark) ? 2 : 0;
    auto& effect = add(1, "bgEffect", "Background Effect", ControlType::Dropdown);
    effect.options = { "Solid", "Acrylic", "Liquid Glass" };
    effect.selectedIndex = (Config::Instance().Get().backgroundEffect == "acrylic") ? 1 : (Config::Instance().Get().backgroundEffect == "liquidglass") ? 2 : 0;
    auto& skin = add(1, "startSkin", "Start Button Skin", ControlType::Dropdown);
    skin.options = { "Default", "Classic", "Modern" };
    skin.selectedIndex = (Config::Instance().Get().startButtonSkin == "classic") ? 1 : (Config::Instance().Get().startButtonSkin == "modern") ? 2 : 0;

    y = PADDING;
    // Animation
    add(2, "animSpeed", "Animation Speed", ControlType::Slider).value = Config::Instance().Get().animationSpeed;
    m_controls.back().min = 0.1f; m_controls.back().max = 3.0f; m_controls.back().step = 0.1f;
    add(2, "magScale", "Magnification Scale", ControlType::Slider).value = Config::Instance().Get().magnificationScale;
    m_controls.back().min = 1.0f; m_controls.back().max = 2.0f; m_controls.back().step = 0.05f;
    add(2, "jumpAnim", "Jump Animation", ControlType::Toggle).boolValue = Config::Instance().Get().jumpAnimation;
    add(2, "genieAnim", "Genie Effect", ControlType::Toggle).boolValue = Config::Instance().Get().genieEffect;
    add(2, "badgePulse", "Badge Pulse", ControlType::Toggle).boolValue = Config::Instance().Get().badgePulse;

    y = PADDING;
    // Performance (Chat 09)
    auto& profile = add(3, "perfProfile", "Performance Profile", ControlType::Dropdown);
    profile.options = { "Eco", "Balanced", "Performance", "Custom" };
    std::string p = Config::Instance().Get().performanceProfile;
    profile.selectedIndex = (p == "eco") ? 0 : (p == "balanced") ? 1 : (p == "performance") ? 2 : 3;

    auto& multi = add(3, "multiMonitor", "Multi-Monitor", ControlType::Dropdown);
    multi.options = { "Primary Only", "All Monitors" };
    multi.selectedIndex = (Config::Instance().Get().multiMonitorMode == "all") ? 1 : 0;

    add(3, "targetFPS", "Target FPS", ControlType::Slider).value = static_cast<float>(Config::Instance().Get().targetFPS);
    m_controls.back().min = 30; m_controls.back().max = 240; m_controls.back().step = 10;
    add(3, "vsync", "V-Sync", ControlType::Toggle).boolValue = Config::Instance().Get().vsync;
    add(3, "adaptive", "Adaptive FPS", ControlType::Toggle).boolValue = Config::Instance().Get().adaptiveFPS;

    y = PADDING;
    // About
    add(4, "about", "DockForge v1.0.0-alpha", ControlType::Label);
    add(4, "exportTheme", "Export Theme...", ControlType::Button).onClick = []() {
        Config::Instance().ExportTheme("DockForge_Theme.json");
        LOG_INFO("Theme exported");
    };
    add(4, "importTheme", "Import Theme...", ControlType::Button).onClick = []() {
        Config::Instance().ImportTheme("DockForge_Theme.json");
        LOG_INFO("Theme imported");
    };
}

void SettingsWindow::SyncValuesFromConfig() {
    for (auto& c : m_controls) {
        if (c.id == "showStartButton") c.boolValue = Config::Instance().Get().showStartButton;
        else if (c.id == "showTrayIcons") c.boolValue = Config::Instance().Get().showTrayIcons;
        else if (c.id == "showFPS") c.boolValue = Config::Instance().Get().showFPS;
        else if (c.id == "jumpLists") c.boolValue = Config::Instance().Get().jumpLists;
        else if (c.id == "thumbnailPreviews") c.boolValue = Config::Instance().Get().thumbnailPreviews;
        else if (c.id == "animSpeed") c.value = Config::Instance().Get().animationSpeed;
        else if (c.id == "magScale") c.value = Config::Instance().Get().magnificationScale;
        else if (c.id == "jumpAnim") c.boolValue = Config::Instance().Get().jumpAnimation;
        else if (c.id == "genieAnim") c.boolValue = Config::Instance().Get().genieEffect;
        else if (c.id == "badgePulse") c.boolValue = Config::Instance().Get().badgePulse;
        else if (c.id == "targetFPS") c.value = static_cast<float>(Config::Instance().Get().targetFPS);
        else if (c.id == "vsync") c.boolValue = Config::Instance().Get().vsync;
        else if (c.id == "adaptive") c.boolValue = Config::Instance().Get().adaptiveFPS;
        else if (c.id == "perfProfile") {
            std::string p = Config::Instance().Get().performanceProfile;
            c.selectedIndex = (p == "eco") ? 0 : (p == "balanced") ? 1 : (p == "performance") ? 2 : 3;
        }
        else if (c.id == "multiMonitor") {
            c.selectedIndex = (Config::Instance().Get().multiMonitorMode == "all") ? 1 : 0;
        }
    }
}

void SettingsWindow::Show() { if (m_hwnd) { ShowWindow(m_hwnd, SW_SHOW); SetForegroundWindow(m_hwnd); m_visible = true; } }
void SettingsWindow::Hide() { if (m_hwnd) { ShowWindow(m_hwnd, SW_HIDE); m_visible = false; } }
void SettingsWindow::Destroy() { if (m_hwnd) { DestroyWindow(m_hwnd); m_hwnd = nullptr; } }
void SettingsWindow::Toggle() { if (m_visible) Hide(); else Show(); }

LRESULT CALLBACK SettingsWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_CREATE) {
        auto* self = static_cast<SettingsWindow*>(reinterpret_cast<LPCREATESTRUCT>(lParam)->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        return 0;
    }
    auto* self = reinterpret_cast<SettingsWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (self) return self->HandleMessage(msg, wParam, lParam);
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT SettingsWindow::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: OnPaint(); return 0;
        case WM_MOUSEMOVE: OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)); return 0;
        case WM_LBUTTONDOWN: OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)); return 0;
        case WM_LBUTTONUP: OnLButtonUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)); return 0;
        case WM_CLOSE: Hide(); return 0;
        case WM_DESTROY: return 0;
        default: return DefWindowProcW(m_hwnd, msg, wParam, lParam);
    }
}

void SettingsWindow::OnPaint() {
    if (!m_renderer || !m_renderer->IsInitialized()) return;
    m_renderer->BeginDraw();
    auto& th = ThemeManager::Instance().GetColors();
    m_renderer->Clear(th.background.r, th.background.g, th.background.b, 1.0f);
    DrawTabs();
    DrawControls();
    m_renderer->EndDraw();
}

void SettingsWindow::DrawTabs() {
    auto& th = ThemeManager::Instance().GetColors();
    m_renderer->FillRect(0, 0, static_cast<float>(TAB_WIDTH), static_cast<float>(WIN_HEIGHT), th.backgroundSecondary.r, th.backgroundSecondary.g, th.backgroundSecondary.b, 1.0f);
    for (size_t i = 0; i < m_categories.size(); ++i) {
        float y = PADDING + i * 40;
        bool active = (static_cast<int>(i) == m_activeCategory);
        if (active) {
            m_renderer->FillRect(4, y, static_cast<float>(TAB_WIDTH - 8), 36, th.accent.r, th.accent.g, th.accent.b, 1.0f);
        }
        std::wstring text(m_categories[i].begin(), m_categories[i].end());
        float tx = 20, ty = y + 10;
        float r = active ? 1.0f : th.text.r, g = active ? 1.0f : th.text.g, b = active ? 1.0f : th.text.b;
        m_renderer->DrawTextLayout(text, tx, ty, r, g, b, 14.0f);
    }
}

void SettingsWindow::DrawControls() {
    auto& th = ThemeManager::Instance().GetColors();
    for (auto& c : m_controls) {
        if (c.category != m_activeCategory) continue;
        switch (c.type) {
            case ControlType::Toggle: DrawToggle(c); break;
            case ControlType::Slider: DrawSlider(c); break;
            case ControlType::Dropdown: DrawDropdown(c); break;
            case ControlType::Button: DrawButton(c); break;
            case ControlType::Label: DrawLabel(c); break;
            case ControlType::Separator:
                m_renderer->FillRect(c.x, c.y + 16, c.w, 1, th.border.r, th.border.g, th.border.b, 1.0f);
                break;
        }
    }
}

void SettingsWindow::DrawToggle(SettingControl& c) {
    auto& th = ThemeManager::Instance().GetColors();
    m_renderer->DrawTextLayout(std::wstring(c.label.begin(), c.label.end()), c.x, c.y + 8, th.text.r, th.text.g, th.text.b, 13.0f);
    float sw = 44, sh = 24, sx = c.x + c.w - sw, sy = c.y + 4;
    m_renderer->DrawRoundedRect(sx, sy, sw, sh, sh / 2, th.controlBg.r, th.controlBg.g, th.controlBg.b, 1.0f);
    if (c.boolValue) {
        m_renderer->FillRect(sx + 2, sy + 2, sw / 2, sh - 4, th.accent.r, th.accent.g, th.accent.b, 1.0f);
    } else {
        m_renderer->FillRect(sx + sw / 2 - 2, sy + 2, sw / 2, sh - 4, th.controlBgHover.r, th.controlBgHover.g, th.controlBgHover.b, 1.0f);
    }
}

void SettingsWindow::DrawSlider(SettingControl& c) {
    auto& th = ThemeManager::Instance().GetColors();
    m_renderer->DrawTextLayout(std::wstring(c.label.begin(), c.label.end()), c.x, c.y + 8, th.text.r, th.text.g, th.text.b, 13.0f);
    float trackY = c.y + 20;
    m_renderer->FillRect(c.x + 180, trackY, c.w - 220, 4, th.controlBg.r, th.controlBg.g, th.controlBg.b, 1.0f);
    float t = (c.value - c.min) / (c.max - c.min);
    float knobX = c.x + 180 + t * (c.w - 220 - 16);
    m_renderer->FillRect(knobX, trackY - 6, 16, 16, th.accent.r, th.accent.g, th.accent.b, 1.0f);
    std::wstring val = std::to_wstring(static_cast<int>(c.value * 100)) + (c.id == "animSpeed" ? L"%" : L"");
    if (c.id == "targetFPS") val = std::to_wstring(static_cast<int>(c.value));
    m_renderer->DrawTextLayout(val, c.x + c.w - 40, c.y + 6, th.textSecondary.r, th.textSecondary.g, th.textSecondary.b, 12.0f);
}

void SettingsWindow::DrawDropdown(SettingControl& c) {
    auto& th = ThemeManager::Instance().GetColors();
    m_renderer->DrawTextLayout(std::wstring(c.label.begin(), c.label.end()), c.x, c.y + 8, th.text.r, th.text.g, th.text.b, 13.0f);
    float bx = c.x + 180, bw = c.w - 200;
    m_renderer->DrawRoundedRect(bx, c.y + 2, bw, 28, 4, th.controlBg.r, th.controlBg.g, th.controlBg.b, 1.0f);
    std::string txt = c.selectedIndex < static_cast<int>(c.options.size()) ? c.options[c.selectedIndex] : "";
    m_renderer->DrawTextLayout(std::wstring(txt.begin(), txt.end()), bx + 10, c.y + 8, th.text.r, th.text.g, th.text.b, 13.0f);
}

void SettingsWindow::DrawButton(SettingControl& c) {
    auto& th = ThemeManager::Instance().GetColors();
    auto bg = c.hovered ? th.accentHover : th.accent;
    m_renderer->DrawRoundedRect(c.x, c.y, 160, 36, 6, bg.r, bg.g, bg.b, 1.0f);
    m_renderer->DrawTextLayout(std::wstring(c.label.begin(), c.label.end()), c.x + 20, c.y + 10, 1.0f, 1.0f, 1.0f, 13.0f);
}

void SettingsWindow::DrawLabel(SettingControl& c) {
    auto& th = ThemeManager::Instance().GetColors();
    m_renderer->DrawTextLayout(std::wstring(c.label.begin(), c.label.end()), c.x, c.y + 8, th.textSecondary.r, th.textSecondary.g, th.textSecondary.b, 14.0f);
}

void SettingsWindow::OnMouseMove(int x, int y) {
    int hit = HitTest(x, y);
    for (auto& c : m_controls) c.hovered = false;
    if (hit >= 0) m_controls[hit].hovered = true;
    if (m_dragging && m_dragControl >= 0 && m_controls[m_dragControl].type == ControlType::Slider) {
        auto& c = m_controls[m_dragControl];
        float trackX = c.x + 180;
        float trackW = c.w - 220;
        float t = static_cast<float>(x - trackX - 8) / (trackW - 16);
        t = std::max(0.0f, std::min(1.0f, t));
        float raw = c.min + t * (c.max - c.min);
        c.value = std::round(raw / c.step) * c.step;
        ApplyControlValue(c);
    }
    InvalidateRect(m_hwnd, nullptr, FALSE);
}

void SettingsWindow::OnLButtonDown(int x, int y) {
    // Tabs
    for (size_t i = 0; i < m_categories.size(); ++i) {
        float ty = PADDING + i * 40;
        if (x < TAB_WIDTH && y >= ty && y < ty + 36) {
            m_activeCategory = static_cast<int>(i);
            InvalidateRect(m_hwnd, nullptr, FALSE);
            return;
        }
    }
    int hit = HitTest(x, y);
    if (hit < 0) return;
    auto& c = m_controls[hit];
    if (c.type == ControlType::Toggle) {
        c.boolValue = !c.boolValue;
        ApplyControlValue(c);
    } else if (c.type == ControlType::Button && c.onClick) {
        c.onClick();
    } else if (c.type == ControlType::Slider) {
        m_dragging = true;
        m_dragControl = hit;
    } else if (c.type == ControlType::Dropdown) {
        c.selectedIndex = (c.selectedIndex + 1) % static_cast<int>(c.options.size());
        ApplyControlValue(c);
    }
    InvalidateRect(m_hwnd, nullptr, FALSE);
}

void SettingsWindow::OnLButtonUp(int x, int y) {
    m_dragging = false;
    m_dragControl = -1;
}

int SettingsWindow::HitTest(int x, int y) {
    for (size_t i = 0; i < m_controls.size(); ++i) {
        auto& c = m_controls[i];
        if (c.category != m_activeCategory) continue;
        if (x >= c.x && x < c.x + c.w && y >= c.y && y < c.y + c.h) return static_cast<int>(i);
    }
    return -1;
}

void SettingsWindow::ApplyControlValue(SettingControl& c) {
    auto& cfg = Config::Instance().GetMutable();
    if (c.id == "showStartButton") cfg.showStartButton = c.boolValue;
    else if (c.id == "showTrayIcons") cfg.showTrayIcons = c.boolValue;
    else if (c.id == "showFPS") cfg.showFPS = c.boolValue;
    else if (c.id == "jumpLists") cfg.jumpLists = c.boolValue;
    else if (c.id == "thumbnailPreviews") cfg.thumbnailPreviews = c.boolValue;
    else if (c.id == "animSpeed") cfg.animationSpeed = c.value;
    else if (c.id == "magScale") cfg.magnificationScale = c.value;
    else if (c.id == "jumpAnim") cfg.jumpAnimation = c.boolValue;
    else if (c.id == "genieAnim") cfg.genieEffect = c.boolValue;
    else if (c.id == "badgePulse") cfg.badgePulse = c.boolValue;
    else if (c.id == "targetFPS") cfg.targetFPS = static_cast<int>(c.value);
    else if (c.id == "vsync") cfg.vsync = c.boolValue;
    else if (c.id == "adaptive") cfg.adaptiveFPS = c.boolValue;
    else if (c.id == "theme") {
        if (c.selectedIndex == 0) ThemeManager::Instance().SetMode(ThemeMode::Auto);
        else if (c.selectedIndex == 1) ThemeManager::Instance().SetMode(ThemeMode::Light);
        else ThemeManager::Instance().SetMode(ThemeMode::Dark);
    } else if (c.id == "bgEffect") {
        cfg.backgroundEffect = (c.selectedIndex == 0) ? "solid" : (c.selectedIndex == 1) ? "acrylic" : "liquidglass";
    } else if (c.id == "startSkin") {
        cfg.startButtonSkin = (c.selectedIndex == 0) ? "default" : (c.selectedIndex == 1) ? "classic" : "modern";
    } else if (c.id == "perfProfile") {
        std::string profiles[] = { "eco", "balanced", "performance", "custom" };
        cfg.performanceProfile = profiles[c.selectedIndex];
        if (c.selectedIndex == 0) PerformanceProfileManager::Instance().SetProfile(PerformanceProfile::PowerSaver);
        else if (c.selectedIndex == 1) PerformanceProfileManager::Instance().SetProfile(PerformanceProfile::Balanced);
        else if (c.selectedIndex == 2) PerformanceProfileManager::Instance().SetProfile(PerformanceProfile::Performance);
        else PerformanceProfileManager::Instance().SetProfile(PerformanceProfile::Adaptive);
    } else if (c.id == "multiMonitor") {
        cfg.multiMonitorMode = (c.selectedIndex == 1) ? "all" : "primary";
    }
    Config::Instance().SaveToFile();
}

void SettingsWindow::RefreshValues() {
    SyncValuesFromConfig();
    InvalidateRect(m_hwnd, nullptr, FALSE);
}