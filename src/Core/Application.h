#pragma once
#include <windows.h>
#include "DockWindow.h"
#include "../Shell/TaskbarHider.h"
#include <memory>

class Application {
public:
    Application();
    ~Application();
    bool Initialize(HINSTANCE hInstance);
    int Run();
    void Shutdown();
private:
    std::unique_ptr<DockWindow> m_dockWindow;
    std::unique_ptr<TaskbarHider> m_taskbarHider;
    bool m_initialized = false;
};
