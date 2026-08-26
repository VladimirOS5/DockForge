#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <map>
#include <mutex>

struct WindowInfo {
    HWND hwnd = nullptr;
    std::wstring processName;
    std::wstring title;
    std::wstring exePath;
    bool isVisible = false;
    bool isMinimized = false;
    bool isForeground = false;
    DWORD pid = 0;
};

class WindowManager {
public:
    static WindowManager& Instance();
    void Initialize();
    void Shutdown();
    void Refresh();
    void OnWindowEvent(HWND hwnd, bool created);
    void OnWindowActivated(HWND hwnd);
    void OnWindowTitleChanged(HWND hwnd);
    
    std::vector<WindowInfo> GetWindowsForProcess(const std::wstring& processName) const;
    std::vector<WindowInfo> GetAllWindows() const;
    bool HasWindowsForProcess(const std::wstring& processName) const;
    int GetWindowCountForProcess(const std::wstring& processName) const;
    
    void ActivateWindow(HWND hwnd);
    void MinimizeWindow(HWND hwnd);
    void RestoreWindow(HWND hwnd);
    void CloseWindow(HWND hwnd);
    void FlashWindow(HWND hwnd);
    
    bool IsWindowValid(HWND hwnd) const;
private:
    WindowManager() = default;
    bool ShouldTrackWindow(HWND hwnd) const;
    void UpdateWindowInfo(HWND hwnd, WindowInfo& info);
    void GetProcessInfo(HWND hwnd, WindowInfo& info);
    
    std::map<HWND, WindowInfo> m_windows;
    mutable std::mutex m_mutex;
};