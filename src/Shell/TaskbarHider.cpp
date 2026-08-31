#include "TaskbarHider.h"
#include "../Utils/Logger.h"
#include <shellapi.h>

TaskbarHider::TaskbarHider() {
    m_hTaskbar = FindWindowW(L"Shell_TrayWnd", nullptr);
    m_hStart = FindWindowW(L"Start", nullptr);
}

TaskbarHider::~TaskbarHider() {
    if (m_hidden) Restore();
}

bool TaskbarHider::GetTaskbarRect(RECT& rect) {
    if (!m_hTaskbar) return false;
    return GetWindowRect(m_hTaskbar, &rect) == TRUE;
}

bool TaskbarHider::Hide() {
    if (!m_hTaskbar) { LOG_ERROR("Shell_TrayWnd not found"); return false; }
    if (m_hidden) return true;

    APPBARDATA abdGet = { sizeof(abdGet) };
    abdGet.hWnd = m_hTaskbar;
    m_originalState = (UINT)SHAppBarMessage(ABM_GETSTATE, &abdGet);
    GetTaskbarRect(m_originalRect);

    HMONITOR hMonitor = MonitorFromWindow(m_hTaskbar, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO mi = { sizeof(mi) };
    GetMonitorInfo(hMonitor, &mi);

    APPBARDATA abd = { sizeof(abd) };
    abd.hWnd = m_hTaskbar;
    abd.uEdge = ABE_BOTTOM;
    abd.lParam = ABS_AUTOHIDE;
    SHAppBarMessage(ABM_SETSTATE, &abd);

    abd.rc.left = mi.rcMonitor.left;
    abd.rc.top = mi.rcMonitor.bottom;
    abd.rc.right = mi.rcMonitor.right;
    abd.rc.bottom = mi.rcMonitor.bottom + 200;
    abd.uEdge = ABE_BOTTOM;
    SHAppBarMessage(ABM_SETPOS, &abd);

    ShowWindow(m_hTaskbar, SW_HIDE);
    if (m_hStart) ShowWindow(m_hStart, SW_HIDE);

    HWND hSecondary = nullptr;
    while ((hSecondary = FindWindowExW(nullptr, hSecondary, L"Shell_SecondaryTrayWnd", nullptr)) != nullptr) {
        ShowWindow(hSecondary, SW_HIDE);
    }

    m_hidden = true;
    LOG_INFO("Taskbar hidden successfully");
    return true;
}

bool TaskbarHider::Restore() {
    if (!m_hTaskbar) return false;
    if (!m_hidden) return true;

    ShowWindow(m_hTaskbar, SW_SHOW);
    if (m_hStart) ShowWindow(m_hStart, SW_SHOW);

    HWND hSecondary = nullptr;
    while ((hSecondary = FindWindowExW(nullptr, hSecondary, L"Shell_SecondaryTrayWnd", nullptr)) != nullptr) {
        ShowWindow(hSecondary, SW_SHOW);
    }

    HMONITOR hMonitor = MonitorFromWindow(m_hTaskbar, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO mi = { sizeof(mi) };
    GetMonitorInfo(hMonitor, &mi);

    APPBARDATA abd = { sizeof(abd) };
    abd.hWnd = m_hTaskbar;
    abd.uEdge = ABE_BOTTOM;
    abd.rc.left = mi.rcMonitor.left;
    abd.rc.top = mi.rcMonitor.bottom - 48;
    abd.rc.right = mi.rcMonitor.right;
    abd.rc.bottom = mi.rcMonitor.bottom;
    SHAppBarMessage(ABM_SETPOS, &abd);

    abd.lParam = m_originalState;
    SHAppBarMessage(ABM_SETSTATE, &abd);

    m_hidden = false;
    LOG_INFO("Taskbar restored successfully");
    return true;
}

bool TaskbarHider::Show() {
    return Restore();
}
