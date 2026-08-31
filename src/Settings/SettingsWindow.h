#pragma once
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <d2d1.h>
#include <windows.h>
#include "../Renderer/D2DRenderer.h"
#include "../Utils/Theme.h"

enum class ControlType { Toggle, Slider, Dropdown, Separator, Label, Button };

struct SettingControl {
    std::string id;
    std::string label;
    ControlType type;
    int category = 0;
    float x = 0, y = 0;
    float min = 0, max = 1, step = 0.1f;
    int w = 0, h = 0;
    union { bool boolValue; int selectedIndex; float value; };
    std::vector<std::string> options;
    std::function<void()> onClick;
    bool hovered = false;
    SettingControl() : boolValue(false) {}
};

struct SettingCategory {
    std::string name;
    std::vector<SettingControl> controls;
};

class SettingsWindow {
public:
    SettingsWindow();
    ~SettingsWindow();
    bool Create(HINSTANCE hInstance);
    void Show();
    void Hide();
    void Toggle();
    void Destroy();
    void OnSettingChanged(std::function<void(const std::string&, const std::string&)> cb);
    void ApplySettings();
    void Update(float deltaTime);
    bool IsVisible() const { return m_visible; }
    static SettingsWindow& Instance();
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);
private:
    void BuildLayout();
    void SyncValuesFromConfig();
    void OnPaint();
    void OnMouseMove(int x, int y);
    void OnLButtonDown(int x, int y);
    void OnLButtonUp(int x, int y);
    void DrawTabs();
    void DrawControls();
    void DrawToggle(const SettingControl& c);
    void DrawSlider(const SettingControl& c);
    void DrawDropdown(const SettingControl& c);
    void DrawButton(const SettingControl& c);
    void DrawLabel(const SettingControl& c);
    int HitTest(int x, int y);
    void ApplyControlValue(SettingControl& c);
    void RefreshValues();
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    HWND m_hwnd = nullptr;
    HINSTANCE m_hInstance = nullptr;
    bool m_visible = false;
    std::unique_ptr<D2DRenderer> m_renderer;
    std::vector<SettingCategory> m_categories;
    std::vector<SettingControl> m_controls;
    int m_activeCategory = 0;
    std::function<void(const std::string&, const std::string&)> m_onChanged;
    bool m_dragging = false;
    int m_dragControl = -1;

    static constexpr int WIN_WIDTH = 900;
    static constexpr int WIN_HEIGHT = 600;
    static constexpr int TAB_WIDTH = 220;
    static constexpr int PADDING = 24;
    static constexpr int ROW_H = 40;
    static constexpr int GAP = 12;
    static constexpr int SECTION_GAP = 20;
};