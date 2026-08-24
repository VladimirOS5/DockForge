#include "WatchdogProcess.h"
#include <tlhelp32.h>
#include <shellapi.h>
#include <filesystem>

#define WATCHDOG_MUTEX L"DockForge_Watchdog_Mutex"

WatchdogProcess::WatchdogProcess() { m_stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr); }
WatchdogProcess::~WatchdogProcess() { Stop(); if (m_stopEvent) CloseHandle(m_stopEvent); }

bool WatchdogProcess::StartMonitoring(const std::wstring& targetProcessName) {
    m_targetName = targetProcessName;
    m_running = true;
    m_restartAttempts = 0;
    return true;
}

void WatchdogProcess::Stop() { m_running = false; if (m_stopEvent) SetEvent(m_stopEvent); }

void WatchdogProcess::Run() {
    HANDLE hMutex = CreateMutexW(nullptr, FALSE, WATCHDOG_MUTEX);
    while (m_running) {
        if (!IsTargetRunning()) {
            RestoreTaskbar();
            if (m_restartAttempts < MAX_RESTART_ATTEMPTS) {
                if (RestartTarget()) m_restartAttempts = 0;
                else m_restartAttempts++;
            }
            Sleep(5000);
        }
        DWORD waitResult = WaitForSingleObject(m_stopEvent, 1000);
        if (waitResult == WAIT_OBJECT_0) break;
    }
    if (hMutex) CloseHandle(hMutex);
}

bool WatchdogProcess::IsTargetRunning() {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return false;
    PROCESSENTRY32W pe32 = { sizeof(pe32) };
    bool found = false;
    if (Process32FirstW(hSnapshot, &pe32)) {
        do { if (_wcsicmp(pe32.szExeFile, m_targetName.c_str()) == 0) { found = true; break; } }
        while (Process32NextW(hSnapshot, &pe32));
    }
    CloseHandle(hSnapshot);
    return found;
}

void WatchdogProcess::RestoreTaskbar() {
    HWND hTaskbar = FindWindowW(L"Shell_TrayWnd", nullptr);
    if (hTaskbar) {
        ShowWindow(hTaskbar, SW_SHOW);
        APPBARDATA abd = {}; abd.cbSize = sizeof(abd); abd.hWnd = hTaskbar; abd.uEdge = ABE_BOTTOM;
        HMONITOR hMonitor = MonitorFromWindow(hTaskbar, MONITOR_DEFAULTTOPRIMARY);
        MONITORINFO mi = { sizeof(mi) }; GetMonitorInfo(hMonitor, &mi);
        abd.rc.left = mi.rcMonitor.left; abd.rc.top = mi.rcMonitor.bottom - 48;
        abd.rc.right = mi.rcMonitor.right; abd.rc.bottom = mi.rcMonitor.bottom;
        SHAppBarMessage(ABM_SETPOS, &abd);
        abd.lParam = ABS_ALWAYSONTOP; SHAppBarMessage(ABM_SETSTATE, &abd);
    }
    HWND hStart = FindWindowW(L"Start", nullptr);
    if (hStart) ShowWindow(hStart, SW_SHOW);
    HWND hSecondary = nullptr;
    while ((hSecondary = FindWindowExW(nullptr, hSecondary, L"Shell_SecondaryTrayWnd", nullptr)) != nullptr)
        ShowWindow(hSecondary, SW_SHOW);
}

bool WatchdogProcess::RestartTarget() {
    wchar_t path[MAX_PATH]; GetModuleFileNameW(nullptr, path, MAX_PATH);
    std::wstring exePath = path;
    size_t lastSlash = exePath.find_last_of(L"\\/");
    if (lastSlash != std::wstring::npos) exePath = exePath.substr(0, lastSlash + 1) + m_targetName;
    SHELLEXECUTEINFOW sei = { sizeof(sei) };
    sei.lpVerb = L"open"; sei.lpFile = exePath.c_str(); sei.nShow = SW_SHOW;
    return ShellExecuteExW(&sei) == TRUE;
}
