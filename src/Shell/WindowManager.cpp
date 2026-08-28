#include "WindowManager.h"
#include "../Utils/Logger.h"
#include <dwmapi.h>
#include <shellapi.h>

WindowManager& WindowManager::Instance() {
    static WindowManager instance;
    return instance;
}

void WindowManager::Initialize() {
    RefreshWindowList();
    LOG_INFO("WindowManager initialized with " + std::to_string(m_windows.size()) + " windows");
}

void WindowManager::Shutdown() {
    m_windows.clear();
}

void WindowManager::RefreshWindowList() {
    m_windows.clear();
    EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(this));
}

BOOL CALLBACK WindowManager::EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    if (!IsWindowVisible(hwnd)) return TRUE;
    if (GetParent(hwnd) != nullptr) return TRUE;
    WindowManager* self = reinterpret_cast<WindowManager*>(lParam);
    WindowInfo info;
    info.hwnd = hwnd;
    wchar_t title[256];
    GetWindowTextW(hwnd, title, 256);
    info.title = title;
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    info.processId = pid;
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (hProcess) {
        wchar_t path[MAX_PATH];
        DWORD size = MAX_PATH;
        if (QueryFullProcessImageNameW(hProcess, 0, path, &size)) {
            info.processPath = path;
            size_t pos = info.processPath.find_last_of(L"\\/");
            if (pos != std::wstring::npos) info.processName = info.processPath.substr(pos + 1);
        }
        CloseHandle(hProcess);
    }
    info.isMinimized = IsIconic(hwnd);
    info.isForeground = (hwnd == GetForegroundWindow());
    self->m_windows.push_back(info);
    return TRUE;
}

std::vector<WindowInfo> WindowManager::GetWindowsForProcess(const std::wstring& processName) const {
    std::vector<WindowInfo> result;
    for (const auto& w : m_windows) {
        if (w.processName == processName) result.push_back(w);
    }
    return result;
}

int WindowManager::GetWindowCountForProcess(const std::wstring& processName) const {
    int count = 0;
    for (const auto& w : m_windows) {
        if (w.processName == processName) ++count;
    }
    return count;
}

void WindowManager::ActivateWindow(HWND hwnd) {
    if (!hwnd || !IsWindow(hwnd)) return;
    if (IsIconic(hwnd)) ShowWindow(hwnd, SW_RESTORE);
    SetForegroundWindow(hwnd);
    ::FlashWindow(hwnd, FALSE);
}

void WindowManager::MinimizeWindow(HWND hwnd) {
    if (hwnd && IsWindow(hwnd)) ShowWindow(hwnd, SW_MINIMIZE);
}

void WindowManager::RestoreWindow(HWND hwnd) {
    if (hwnd && IsWindow(hwnd)) ShowWindow(hwnd, SW_RESTORE);
}

void WindowManager::CloseWindow(HWND hwnd) {
    if (hwnd && IsWindow(hwnd)) PostMessageW(hwnd, WM_CLOSE, 0, 0);
}

void WindowManager::OnWindowEvent(HWND hwnd, bool created) {
    RefreshWindowList();
}

void WindowManager::OnWindowActivated(HWND hwnd) {
    for (auto& w : m_windows) {
        w.isForeground = (w.hwnd == hwnd);
    }
}

void WindowManager::OnWindowTitleChanged(HWND hwnd) {
    for (auto& w : m_windows) {
        if (w.hwnd == hwnd) {
            wchar_t title[256];
            GetWindowTextW(hwnd, title, 256);
            w.title = title;
            break;
        }
    }
}

bool WindowManager::IsWindowValid(HWND hwnd) const {
    return hwnd && IsWindow(hwnd);
}

std::wstring WindowManager::GetWindowTitle(HWND hwnd) const {
    if (!hwnd || !IsWindow(hwnd)) return L"";
    wchar_t title[256];
    GetWindowTextW(hwnd, title, 256);
    return title;
}

std::wstring WindowManager::GetProcessName(HWND hwnd) const {
    if (!hwnd || !IsWindow(hwnd)) return L"";
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hProcess) return L"";
    wchar_t path[MAX_PATH];
    DWORD size = MAX_PATH;
    std::wstring result;
    if (QueryFullProcessImageNameW(hProcess, 0, path, &size)) {
        result = path;
        size_t pos = result.find_last_of(L"\\/");
        if (pos != std::wstring::npos) result = result.substr(pos + 1);
    }
    CloseHandle(hProcess);
    return result;
}

void WindowManager::FlashWindow(HWND hwnd) {
    if (hwnd && IsWindow(hwnd)) {
        FLASHWINFO fi = { sizeof(fi) };
        fi.hwnd = hwnd;
        fi.dwFlags = FLASHW_ALL | FLASHW_TIMERNOFG;
        fi.uCount = 3;
        fi.dwTimeout = 0;
        FlashWindowEx(&fi);
    }
}
