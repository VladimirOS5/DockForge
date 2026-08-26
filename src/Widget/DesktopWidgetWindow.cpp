#include "DesktopWidgetWindow.h"
#include "../Utils/Logger.h"

bool DesktopWidgetWindow::Create(HINSTANCE hInstance, WidgetBase* widget, int x, int y) {
    m_widget = widget;
    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"DockForgeDesktopWidget";
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    RegisterClassExW(&wc);
    m_hwnd = CreateWindowExW(WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        L"DockForgeDesktopWidget", L"Widget", WS_POPUP,
        x, y, 200, 120, nullptr, nullptr, hInstance, this);
    return m_hwnd != nullptr;
}

void DesktopWidgetWindow::Show() { if (m_hwnd) ShowWindow(m_hwnd, SW_SHOW); }
void DesktopWidgetWindow::Hide() { if (m_hwnd) ShowWindow(m_hwnd, SW_HIDE); }
void DesktopWidgetWindow::Destroy() { if (m_hwnd) { DestroyWindow(m_hwnd); m_hwnd = nullptr; } }

LRESULT DesktopWidgetWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_CREATE) {
        auto* self = static_cast<DesktopWidgetWindow*>(reinterpret_cast<LPCREATESTRUCT>(lParam)->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        return 0;
    }
    auto* self = reinterpret_cast<DesktopWidgetWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!self) return DefWindowProcW(hwnd, msg, wParam, lParam);
    if (msg == WM_PAINT) {
        PAINTSTRUCT ps; BeginPaint(hwnd, &ps);
        EndPaint(hwnd, &ps);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}