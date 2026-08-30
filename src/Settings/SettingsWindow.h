#pragma once
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <d2d1.h>
#include <windows.h>

enum class ControlType { Toggle, Slider, Combo };

struct SettingControl {
    std::string id;
    std::string label;
    ControlType type;
    std::string category;
    D2D1_RECT_F rect = {};
    union {
        bool boolValue;
        int selectedIndex;
        float value;
    };
    float min = 0, max = 1;
    int w = 0, h = 0;
};

struct SettingCategory {
    std::string name;
    std::vector<SettingControl> controls;
};

class D2DRenderer;

class SettingsWindow {
public:
    SettingsWindow();
    ~SettingsWindow();
    bool Create(HINSTANCE hInstance);
    void Show();
    void Hide();
    void Destroy();
    void OnSettingChanged(std::function<void(const std::string&, const std::string&)> cb);
    void ApplySettings();
    void Update(float deltaTime);
    bool IsVisible() const { return m_visible; }
    static SettingsWindow& Instance();
private:
    void BuildLayout();
    void OnPaint();
    void DrawTabs();
    void DrawControls();
    void DrawToggle(const SettingControl& c);
    void DrawSlider(const SettingControl& c);
    void DrawCombo(const SettingControl& c);
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    HWND m_hwnd = nullptr;
    HINSTANCE m_hInstance = nullptr;
    bool m_visible = false;
    std::unique_ptr<D2DRenderer> m_renderer;
    std::vector<SettingCategory> m_categories;
    std::vector<SettingControl> m_controls;
    int m_activeCategory = 0;
    std::function<void(const std::string&, const std::string&)> m_onChanged;

    static constexpr int WIN_WIDTH = 900;
    static constexpr int WIN_HEIGHT = 600;
    static constexpr int TAB_WIDTH = 220;
    static constexpr int PADDING = 24;
    static constexpr int ROW_H = 40;
    static constexpr int GAP = 12;
    static constexpr int SECTION_GAP = 20;
};
