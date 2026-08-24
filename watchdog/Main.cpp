#include <windows.h>
#include "WatchdogProcess.h"

int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
    WatchdogProcess watchdog;
    if (!watchdog.StartMonitoring(L"DockForge.exe")) return 1;
    watchdog.Run();
    return 0;
}
