#pragma once
#include <windows.h>
#include <vector>
#include <memory>
#include <string>

class DockWindow;

struct MonitorInfo {
    HMONITOR hMonitor = nullptr;
    RECT workArea = {};
    RECT monitorRect = {};
    bool isPrimary = false;
    std::wstring deviceName;
};

class MonitorManager {
public:
    static MonitorManager& Instance();
    void Initialize(HINSTANCE hInstance);
    void Shutdown();
    void Refresh();
    const std::vector<MonitorInfo>& GetMonitors() const { return m_monitors; }
    size_t GetPrimaryIndex() const;
    void ShowAll();
    void HideAll();
    void UpdateAll(float deltaTime);
    void RenderAll();
    std::vector<DockWindow*> GetDockWindows() const;
private:
    MonitorManager() = default;
    static BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData);
    void CreateDockWindows(HINSTANCE hInstance);
    void DestroyDockWindows();
    
    std::vector<MonitorInfo> m_monitors;
    std::vector<std::unique_ptr<DockWindow>> m_docks;
    HINSTANCE m_hInstance = nullptr;
};