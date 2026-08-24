#include <windows.h>
#include "Application.h"
#include "../Utils/Logger.h"

#define INSTANCE_MUTEX L"DockForge_SingleInstance_Mutex"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int) {
    HANDLE hMutex = CreateMutexW(nullptr, TRUE, INSTANCE_MUTEX);
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        MessageBoxW(nullptr, L"DockForge is already running.", L"DockForge", MB_OK | MB_ICONINFORMATION);
        if (hMutex) CloseHandle(hMutex);
        return 0;
    }
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    Application app;
    if (!app.Initialize(hInstance)) {
        if (hMutex) { ReleaseMutex(hMutex); CloseHandle(hMutex); }
        return 1;
    }
    int result = app.Run();
    app.Shutdown();
    if (hMutex) { ReleaseMutex(hMutex); CloseHandle(hMutex); }
    return result;
}
