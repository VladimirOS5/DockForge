#pragma once
#include <windows.h>
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include "../Renderer/D2DRenderer.h"

enum class ControlType { Toggle, Slider, Dropdown, Button, Label, Separator };

struct SettingControl {
    std::string id;
    std::string label;
    ControlType type;
    int category = 0;
    float value = 0;
    float min = 0, max = 1, step = 0.1f;
    bool boolValue = false;
    std::vector<std::string> options;
    int selectedIndex = 0;
    std::function<void()> onClick;
    float x = 0, y = 0, w = 0, h = 0;
    bool hovered = false;
    bool open = false;
};

class SettingsWindow {
public:
    static SettingsWindow& Instance();
    bool Create(HINSTANCE hInstance);
    void Show();
    void Hide();
    void Destroy();
    bool IsVisible() const { return m_visible; }
    void Toggle();
    void RefreshValues();
    HWND GetHwnd() const { return m_hwnd; }
private:
    SettingsWindow() = default;
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);
    void OnPaint();
    void BuildLayout();
    void DrawTabs();
    void DrawControls();
    void DrawToggle(SettingControl& c);
    void DrawSlider(SettingControl& c);
    void DrawDropdown(SettingControl& c);
    void DrawButton(SettingControl& c);
    void DrawLabel(SettingControl& c);
    void OnMouseMove(int x, int y);
    void OnLButtonDown(int x, int y);
    void OnLButtonUp(int x, int y);
    int HitTest(int x, int y);
    void ApplyControlValue(SettingControl& c);
    void SyncValuesFromConfig();
    
    HWND m_hwnd = nullptr;
    HINSTANCE m_hInstance = nullptr;
    std::unique_ptr<D2DRenderer> m_renderer;
    std::vector<std::string> m_categories = { "General", "Appearance", "Animation", "Performance", "About" };
    int m_activeCategory = 0;
    std::vector<SettingControl> m_controls;
    bool m_visible = false;
    bool m_dragging = false;
    int m_dragControl = -1;
    float m_dragStartX = 0;
    float m_dragStartValue = 0;
    
    static constexpr int WIN_WIDTH = 720;
    static constexpr int WIN_HEIGHT = 520;
    static constexpr int TAB_WIDTH = 170;
    static constexpr int PADDING = 24;
    static constexpr int ROW_H = 44;
};#pragma once
#include <windows.h>
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include "../Renderer/D2DRenderer.h"

enum class ControlType { Toggle, Slider, Dropdown, Button, Label, Separator };

struct SettingControl {
    std::string id;
    std::string label;
    ControlType type;
    int category = 0;
    float value = 0;
    float min = 0, max = 1, step = 0.1f;
    bool boolValue = false;
    std::vector<std::string> options;
    int selectedIndex = 0;
    std::function<void()> onClick;
    float x = 0, y = 0, w = 0, h = 0;
    bool hovered = false;
    bool open = false;
};

class SettingsWindow {
public:
    static SettingsWindow& Instance();
    bool Create(HINSTANCE hInstance);
    void Show();
    void Hide();
    void Destroy();
    bool IsVisible() const { return m_visible; }
    void Toggle();
    void RefreshValues();
    HWND GetHwnd() const { return m_hwnd; }
private:
    SettingsWindow() = default;
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);
    void OnPaint();
    void BuildLayout();
    void DrawTabs();
    void DrawControls();
    void DrawToggle(SettingControl& c);
    void DrawSlider(SettingControl& c);
    void DrawDropdown(SettingControl& c);
    void DrawButton(SettingControl& c);
    void DrawLabel(SettingControl& c);
    void OnMouseMove(int x, int y);
    void OnLButtonDown(int x, int y);
    void OnLButtonUp(int x, int y);
    int HitTest(int x, int y);
    void ApplyControlValue(SettingControl& c);
    void SyncValuesFromConfig();
    
    HWND m_hwnd = nullptr;
    HINSTANCE m_hInstance = nullptr;
    std::unique_ptr<D2DRenderer> m_renderer;
    std::vector<std::string> m_categories = { "General", "Appearance", "Animation", "Performance", "About" };
    int m_activeCategory = 0;
    std::vector<SettingControl> m_controls;
    bool m_visible = false;
    bool m_dragging = false;
    int m_dragControl = -1;
    float m_dragStartX = 0;
    float m_dragStartValue = 0;
    
    static constexpr int WIN_WIDTH = 720;
    static constexpr int WIN_HEIGHT = 520;
    static constexpr int TAB_WIDTH = 170;
    static constexpr int PADDING = 24;
    static constexpr int ROW_H = 44;
};