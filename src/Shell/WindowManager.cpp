#include "WindowManager.h"
#include "../Utils/Logger.h"

WindowManager& WindowManager::Instance() {
    static WindowManager instance;
    return instance;
}

void WindowManager::Initialize() {
    Refresh();
    LOG_INFO("WindowManager initialized with " + std::to_string(m_windows.size()) + " windows");
}

void WindowManager::Shutdown() {
    m_windows.clear();
}

void WindowManager::Refresh() {
    m_windows.clear();
    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        auto* self = reinterpret_cast<WindowManager*>(lParam);
        if (!IsWindowVisible(hwnd)) return TRUE;
        if (GetParent(hwnd) != nullptr) return TRUE;
        WindowInfo info;
        info.hwnd = hwnd;
        wchar_t title[256];
        GetWindowTextW(hwnd, title, 256);
        info.title = title;
        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);
        info.pid = pid;
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (hProcess) {
            wchar_t path[MAX_PATH];
            DWORD size = MAX_PATH;
            if (QueryFullProcessImageNameW(hProcess, 0, path, &size)) {
                info.exePath = path;
                size_t pos = info.exePath.find_last_of(L"\\/");
                if (pos != std::wstring::npos) info.processName = info.exePath.substr(pos + 1);
            }
            CloseHandle(hProcess);
        }
        info.isMinimized = IsIconic(hwnd);
        info.isForeground = (hwnd == GetForegroundWindow());
        self->m_windows[hwnd] = info;
        return TRUE;
    }, reinterpret_cast<LPARAM>(this));
}

std::vector<WindowInfo> WindowManager::GetWindowsForProcess(const std::wstring& processName) const {
    std::vector<WindowInfo> result;
    for (const auto& pair : m_windows) {
        if (pair.second.processName == processName) result.push_back(pair.second);
    }
    return result;
}

int WindowManager::GetWindowCountForProcess(const std::wstring& processName) const {
    int count = 0;
    for (const auto& pair : m_windows) {
        if (pair.second.processName == processName) ++count;
    }
    return count;
}

std::vector<WindowInfo> WindowManager::GetAllWindows() const {
    std::vector<WindowInfo> result;
    for (const auto& pair : m_windows) {
        result.push_back(pair.second);
    }
    return result;
}

void WindowManager::OnWindowEvent(HWND hwnd, bool created) {
    Refresh();
}

void WindowManager::OnWindowActivated(HWND hwnd) {
    for (auto& pair : m_windows) {
        pair.second.isForeground = (pair.second.hwnd == hwnd);
    }
}

void WindowManager::OnWindowTitleChanged(HWND hwnd) {
    for (auto& pair : m_windows) {
        if (pair.second.hwnd == hwnd) {
            wchar_t title[256];
            GetWindowTextW(hwnd, title, 256);
            pair.second.title = title;
            break;
        }
    }
}

bool WindowManager::IsWindowValid(HWND hwnd) const {
    return hwnd && IsWindow(hwnd);
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

void WindowManager::CloseWindow(HWND hwnd) {
    if (hwnd && IsWindow(hwnd)) {
        PostMessageW(hwnd, WM_CLOSE, 0, 0);
    }
}
