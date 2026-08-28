#include "DPIHelper.h"
#include <shellscalingapi.h>
#include <windows.h>

float DPIHelper::GetSystemScale() {
    HMONITOR hMonitor = MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTOPRIMARY);
    UINT dpiX = 96, dpiY = 96;
    HRESULT hr = GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);
    if (SUCCEEDED(hr)) return static_cast<float>(dpiX) / 96.0f;
    return 1.0f;
}

float DPIHelper::GetScaleForWindow(HWND hwnd) {
    if (!hwnd) return GetSystemScale();
    HMONITOR hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    UINT dpiX = 96, dpiY = 96;
    HRESULT hr = GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);
    if (SUCCEEDED(hr)) return static_cast<float>(dpiX) / 96.0f;
    return 1.0f;
}

int DPIHelper::ScaleInt(int value, float scale) {
    return static_cast<int>(value * scale);
}

float DPIHelper::ScaleFloat(float value, float scale) {
    return value * scale;
}

void DPIHelper::EnablePerMonitorV2() {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
}
