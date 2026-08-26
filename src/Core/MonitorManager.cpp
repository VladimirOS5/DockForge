#include "MonitorManager.h"
#include "DockWindow.h"
#include "../Utils/Logger.h"
#include "../Utils/Config.h"
#include <algorithm>

MonitorManager& MonitorManager::Instance() {
    static MonitorManager instance;
    return instance;
}

void MonitorManager::Initialize(HINSTANCE hInstance) {
    m_hInstance = hInstance;
    Refresh();
    CreateDockWindows(hInstance);
    LOG_INFO("MonitorManager initialized with " + std::to_string(m_docks.size()) + " dock(s)");
}

void MonitorManager::Shutdown() {
    DestroyDockWindows();
    m_monitors.clear();
    LOG_INFO("MonitorManager shutdown");
}

void MonitorManager::Refresh() {
    m_monitors.clear();
    EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, reinterpret_cast<LPARAM>(this));
    std::sort(m_monitors.begin(), m_monitors.end(), [](const MonitorInfo& a, const MonitorInfo& b) {
        return a.isPrimary > b.isPrimary;
    });
}

BOOL CALLBACK MonitorManager::MonitorEnumProc(HMONITOR hMonitor, HDC, LPRECT, LPARAM dwData) {
    auto* self = reinterpret_cast<MonitorManager*>(dwData);
    MONITORINFOEXW mi = { sizeof(mi) };
    if (GetMonitorInfoW(hMonitor, &mi)) {
        MonitorInfo info;
        info.hMonitor = hMonitor;
        info.workArea = mi.rcWork;
        info.monitorRect = mi.rcMonitor;
        info.isPrimary = (mi.dwFlags & MONITORINFOF_PRIMARY) != 0;
        info.deviceName = mi.szDevice;
        self->m_monitors.push_back(info);
    }
    return TRUE;
}

size_t MonitorManager::GetPrimaryIndex() const {
    for (size_t i = 0; i < m_monitors.size(); ++i) {
        if (m_monitors[i].isPrimary) return i;
    }
    return 0;
}

void MonitorManager::CreateDockWindows(HINSTANCE hInstance) {
    DestroyDockWindows();
    auto& cfg = Config::Instance().Get();
    bool allMonitors = cfg.multiMonitorMode == "all";
    
    for (size_t i = 0; i < m_monitors.size(); ++i) {
        if (!allMonitors && !m_monitors[i].isPrimary) continue;
        
        auto dock = std::make_unique<DockWindow>();
        if (dock->Create(hInstance, &m_monitors[i])) {
            dock->Show();
            m_docks.push_back(std::move(dock));
            LOG_INFO("Dock created on monitor: " + std::string(m_monitors[i].deviceName.begin(), m_monitors[i].deviceName.end()));
        } else {
            LOG_ERROR("Failed to create dock on monitor");
        }
    }
}

void MonitorManager::DestroyDockWindows() {
    for (auto& dock : m_docks) {
        if (dock) dock->RequestQuit();
    }
    m_docks.clear();
}

void MonitorManager::ShowAll() {
    for (auto& dock : m_docks) if (dock) dock->Show();
}

void MonitorManager::HideAll() {
    for (auto& dock : m_docks) if (dock) dock->Hide();
}

void MonitorManager::UpdateAll(float deltaTime) {
    for (auto& dock : m_docks) {
        if (dock && dock->IsRunning()) {
            dock->Update(deltaTime);
        }
    }
}

void MonitorManager::RenderAll() {
    for (auto& dock : m_docks) {
        if (dock && dock->IsRunning()) {
            dock->Render();
        }
    }
}

std::vector<DockWindow*> MonitorManager::GetDockWindows() const {
    std::vector<DockWindow*> result;
    for (const auto& dock : m_docks) result.push_back(dock.get());
    return result;
}