#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <functional>

struct TrayIconInfo {
    HWND hwnd = nullptr;
    UINT id = 0;
    UINT callbackMessage = 0;
    HICON hIcon = nullptr;
    std::wstring tooltip;
    bool visible = true;
};

class TrayIconManager {
public:
    static TrayIconManager& Instance();
    void Initialize(HINSTANCE hInstance = nullptr);  // Optional owner for compatibility
    void Shutdown();
    void Refresh();
    std::vector<TrayIconInfo> GetIcons() const;
    void SendMouseEvent(const TrayIconInfo& icon, DWORD message);
    void SetCallback(std::function<void()> onChange);
private:
    TrayIconManager() = default;
    HWND FindTrayToolbar();
    bool ReadTrayButtons(HWND hToolbar, std::vector<TrayIconInfo>& icons);
    
    std::vector<TrayIconInfo> m_icons;
    std::function<void()> m_callback;
    UINT_PTR m_timerId = 0;
    static void CALLBACK TimerProc(HWND hwnd, UINT msg, UINT_PTR id, DWORD time);
};