#pragma once
#include <windows.h>

// Per-monitor DPI awareness helpers
class DPIHelper {
public:
    static float GetScaleForWindow(HWND hwnd);
    static float GetScaleForMonitor(HMONITOR hMonitor);
    static float GetSystemScale();
    static int ScaleInt(int value, float dpiScale);
    static float ScaleFloat(float value, float dpiScale);
    static void EnablePerMonitorV2();
    static bool IsPerMonitorAware();

    // High-DPI window message handlers
    static void HandleDpiChange(HWND hwnd, WPARAM wParam, LPARAM lParam);
    static RECT ScaleRect(const RECT& rc, float fromDpi, float toDpi);
};
