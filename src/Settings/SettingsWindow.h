#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <functional>
#include "../Renderer/D2DRenderer.h"

enum class ControlType { Toggle, Slider, Dropdown, ColorPicker, Button, Separator, Label };

struct SettingControl {
    ControlType type;
    std::string label;
    std::string key;
    float minVal = 0, maxVal = 1, step = 0.01f;
    std::vector<std::string> options;
    std::function<void()> onClick;
    bool* boolTarget = nullptr;
    float* floatTarget = nullptr;
    int* intTarget = nullptr;
    std::string* stringTarget = nullptr;
    int currentIndex = 0;
    float currentFloat = 0;
    bool currentBool = false;
    float x = 0, y = 0, width = 200, height = 30;
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
    void Render();
    void OnMouseMove(int x, int y);
    void OnLButtonDown(int x, int y);
    void OnLButtonUp(int x, int y);
    HWND GetHwnd() const { return m_hwnd; }
    void RefreshFromConfig();
private:
    SettingsWindow() = default;
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);
    void BuildLayout();
    void SyncValuesFromConfig();
    void DrawControl(const SettingControl& c);
    void UpdateControlAnimation(SettingControl& c, float deltaTime);

    HWND m_hwnd = nullptr;
    HINSTANCE m_hInstance = nullptr;
    bool m_visible = false;
    std::unique_ptr<D2DRenderer> m_renderer;
    std::vector<SettingControl> m_controls;
    int m_activeTab = 0;
    int m_hoveredControl = -1;
    int m_pressedControl = -1;
    float m_scrollY = 0;
    float m_targetScrollY = 0;
    float m_tabAnimX = 0;

    static constexpr int WIN_WIDTH = 500;
    static constexpr int WIN_HEIGHT = 600;
    static constexpr int PADDING = 20;
    static constexpr int ROW_H = 40;
    static constexpr int TAB_WIDTH = 120;
};
