#pragma once
#include <windows.h>
#include "../Plugin/PluginManager.h"

class DesktopWidgetWindow {
public:
    bool Create(HINSTANCE hInstance, WidgetBase* widget, int x, int y);
    void Show();
    void Hide();
    void Destroy();
private:
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
    HWND m_hwnd = nullptr;
    WidgetBase* m_widget = nullptr;
};