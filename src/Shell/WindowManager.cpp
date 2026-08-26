#include "WindowManager.h"
#include "../Utils/Logger.h"
#include <tlhelp32.h>
#include <psapi.h>
#include <shellapi.h>
#include <mutex>

WindowManager& WindowManager::Instance() {
    static WindowManager instance;
    return instance;
}

void WindowManager::Initialize() {
    Refresh();
    LOG_INFO("WindowManager initialized with " + std::to_string(m_windows.size()) + " windows");
}

void WindowManager::Shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_windows.clear();
    LOG_INFO("WindowManager shutdown");
}

void WindowManager::Refresh() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_windows.clear();
    
    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        auto* self = reinterpret_cast<WindowManager*>(lParam);
        if (self->ShouldTrackWindow(hwnd)) {
            WindowInfo info;
            info.hwnd = hwnd;
            self->UpdateWindowInfo(hwnd, info);
            self->m_windows[hwnd] = info;
        }
        return TRUE;
    }, reinterpret_cast<LPARAM>(this));
}

bool WindowManager::ShouldTrackWindow(HWND hwnd) const {
    if (!IsWindow(hwnd)) return false;
    if (!IsWindowVisible(hwnd)) return false;
    
    LONG exStyle = GetWindowLongW(hwnd, GWL_EXSTYLE);
    if (exStyle & WS_EX_TOOLWINDOW) return false;
    if (exStyle & WS_EX_NOACTIVATE) return false;
    
    wchar_t className[256];
    GetClassNameW(hwnd, className, 256);
    if (wcscmp(className, L"DockForgeDockWindow") == 0) return false;
    if (wcscmp(className, L"DockForgeShellHook") == 0) return false;
    
    RECT rc;
    GetWindowRect(hwnd, &rc);
    if ((rc.right - rc.left) < 100 || (rc.bottom - rc.top) < 50) return false;
    
    return true;
}

void WindowManager::UpdateWindowInfo(HWND hwnd, WindowInfo& info) {
    wchar_t title[512];
    GetWindowTextW(hwnd, title, 512);
    info.title = title;
    
    info.isVisible = IsWindowVisible(hwnd);
    info.isMinimized = IsIconic(hwnd);
    info.isForeground = (GetForegroundWindow() == hwnd);
    
    GetProcessInfo(hwnd, info);
}

void WindowManager::GetProcessInfo(HWND hwnd, WindowInfo& info) {
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    info.pid = pid;
    
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (hProcess) {
        wchar_t path[MAX_PATH];
        if (GetModuleFileNameExW(hProcess, nullptr, path, MAX_PATH)) {
            info.exePath = path;
            size_t pos = info.exePath.find_last_of(L"\\/");
            if (pos != std::wstring::npos) {
                info.processName = info.exePath.substr(pos + 1);
            } else {
                info.processName = info.exePath;
            }
        }
        CloseHandle(hProcess);
    }
}

void WindowManager::OnWindowEvent(HWND hwnd, bool created) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (created) {
        if (ShouldTrackWindow(hwnd)) {
            WindowInfo info;
            info.hwnd = hwnd;
            UpdateWindowInfo(hwnd, info);
            m_windows[hwnd] = info;
            LOG_DEBUG("Window created: " + std::string(info.title.begin(), info.title.end()));
        }
    } else {
        auto it = m_windows.find(hwnd);
        if (it != m_windows.end()) {
            LOG_DEBUG("Window destroyed: " + std::string(it->second.title.begin(), it->second.title.end()));
            m_windows.erase(it);
        }
    }
}

void WindowManager::OnWindowActivated(HWND hwnd) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& [h, info] : m_windows) {
        info.isForeground = (h == hwnd);
    }
    if (m_windows.count(hwnd)) {
        m_windows[hwnd].isMinimized = false;
    }
}

void WindowManager::OnWindowTitleChanged(HWND hwnd) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_windows.find(hwnd);
    if (it != m_windows.end()) {
        wchar_t title[512];
        GetWindowTextW(hwnd, title, 512);
        it->second.title = title;
    }
}

std::vector<WindowInfo> WindowManager::GetWindowsForProcess(const std::wstring& processName) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<WindowInfo> result;
    for (const auto& [hwnd, info] : m_windows) {
        if (_wcsicmp(info.processName.c_str(), processName.c_str()) == 0) {
            result.push_back(info);
        }
    }
    return result;
}

std::vector<WindowInfo> WindowManager::GetAllWindows() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<WindowInfo> result;
    for (const auto& [hwnd, info] : m_windows) {
        result.push_back(info);
    }
    return result;
}

bool WindowManager::HasWindowsForProcess(const std::wstring& processName) const {
    return GetWindowCountForProcess(processName) > 0;
}

int WindowManager::GetWindowCountForProcess(const std::wstring& processName) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    int count = 0;
    for (const auto& [hwnd, info] : m_windows) {
        if (_wcsicmp(info.processName.c_str(), processName.c_str()) == 0) {
            ++count;
        }
    }
    return count;
}

void WindowManager::ActivateWindow(HWND hwnd) {
    if (!IsWindow(hwnd)) return;
    if (IsIconic(hwnd)) {
        ShowWindow(hwnd, SW_RESTORE);
    }
    SetForegroundWindow(hwnd);
    FlashWindow(hwnd, FALSE);
}

void WindowManager::MinimizeWindow(HWND hwnd) {
    if (IsWindow(hwnd)) ShowWindow(hwnd, SW_MINIMIZE);
}

void WindowManager::RestoreWindow(HWND hwnd) {
    if (IsWindow(hwnd)) ShowWindow(hwnd, SW_RESTORE);
}

void WindowManager::CloseWindow(HWND hwnd) {
    if (IsWindow(hwnd)) {
        PostMessageW(hwnd, WM_CLOSE, 0, 0);
    }
}

void WindowManager::FlashWindow(HWND hwnd) {
    if (IsWindow(hwnd)) {
        FLASHWINFO fi = { sizeof(fi) };
        fi.hwnd = hwnd;
        fi.dwFlags = FLASHW_ALL | FLASHW_TIMERNOFG;
        fi.uCount = 3;
        fi.dwTimeout = 0;
        ::FlashWindowEx(&fi);
    }
}

bool WindowManager::IsWindowValid(HWND hwnd) const {
    return IsWindow(hwnd);
}