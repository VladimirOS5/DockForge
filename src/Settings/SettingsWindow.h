#pragma once
#include <windows.h>
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include "../Renderer/D2DRenderer.h"

enum class ControlType { Toggle, Slider, Dropdown, ColorPicker, Button, Separator, Label };

struct SettingControl {
    ControlType type;
    std::string label;
    std::string key;
    std::string id;
    int category = 0;
    bool boolValue = false;
    int selectedIndex = 0;
    float value = 0.0f;
    float min = 0.0f;
    float max = 1.0f;
    float step = 0.01f;
    std::vector<std::string> options;
    std::function<void()> onClick;
    float currentFloat = 0;
    bool currentBool = false;
    int currentIndex = 0;
    D2D1_COLOR_F currentColor = {0,0,0,1};
    float x = 0, y = 0, width = 200, height = 30;
    float w = 200, h = 30;
    bool hovered = false;
    bool pressed = false;
    float animProgress = 0;
};

class SettingsWindow {
public:
    static SettingsWindow& Instance();
    bool Create(HINSTANCE hInstance);
    void Destroy();
    void Show();
    void Hide();
    void Toggle();
    bool IsVisible() const;
    void Update(float deltaTime);
    void OnResize(int width, int height);
    void OnMouseMove(int x, int y);
    void OnLButtonDown(int x, int y);
    void OnLButtonUp(int x, int y);
    HWND GetHwnd() const { return m_hwnd; }
    void RefreshFromConfig();
    void RefreshValues();
    int HitTest(int x, int y);
    void ApplyControlValue(SettingControl& c);
    void DrawTabs();
    void DrawControls();
    void DrawToggle(SettingControl& c);
    void DrawSlider(SettingControl& c);
    void DrawDropdown(SettingControl& c);
    void DrawButton(SettingControl& c);
    void DrawLabel(SettingControl& c);
    void OnPaint();
private:
    SettingsWindow() = default;
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);
    void BuildLayout();
    void SyncValuesFromConfig();
    void DrawControl(SettingControl& c);
    void UpdateControlAnimation(SettingControl& c, float deltaTime);

    HWND m_hwnd = nullptr;
    HINSTANCE m_hInstance = nullptr;
    bool m_visible = false;
    std::unique_ptr<D2DRenderer> m_renderer;
    std::vector<SettingControl> m_controls;
    std::vector<std::string> m_categories = { "General", "Appearance", "Animation", "Performance", "About" };
    int m_activeCategory = 0;
    int m_activeTab = 0;
    int m_hoveredControl = -1;
    int m_pressedControl = -1;
    bool m_dragging = false;
    int m_dragControl = -1;
    static constexpr int WIN_WIDTH = 900;
    static constexpr int WIN_HEIGHT = 600;
    static constexpr int TAB_WIDTH = 180;
    static constexpr int PADDING = 24;
};
