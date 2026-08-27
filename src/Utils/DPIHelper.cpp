#include "DPIHelper.h"
#include "../Utils/Logger.h"

float DPIHelper::GetScaleForWindow(HWND hwnd) {
    UINT dpi = GetDpiForWindow(hwnd);
    return dpi / 96.0f;
}

float DPIHelper::GetScaleForMonitor(HMONITOR hMonitor) {
    UINT dpiX = 96, dpiY = 96;
    HRESULT hr = GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);
    if (FAILED(hr)) return 1.0f;
    return dpiX / 96.0f;
}

float DPIHelper::GetSystemScale() {
    UINT dpi = GetDpiForSystem();
    return dpi / 96.0f;
}

int DPIHelper::ScaleInt(int value, float dpiScale) {
    return static_cast<int>(value * dpiScale);
}

float DPIHelper::ScaleFloat(float value, float dpiScale) {
    return value * dpiScale;
}

void DPIHelper::EnablePerMonitorV2() {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    LOG_INFO("DPI: Per-Monitor V2 awareness enabled");
}

bool DPIHelper::IsPerMonitorAware() {
    DPI_AWARENESS_CONTEXT ctx = GetThreadDpiAwarenessContext();
    return (ctx == DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) ||
           (ctx == DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
}

void DPIHelper::HandleDpiChange(HWND hwnd, WPARAM wParam, LPARAM lParam) {
    UINT newDpi = static_cast<UINT>(LOWORD(wParam));
    const RECT* const prcNewWindow = reinterpret_cast<const RECT*>(lParam);

    float newScale = newDpi / 96.0f;
    LOG_INFO("DPI change: new DPI=" + std::to_string(newDpi) + " scale=" + std::to_string(newScale));

    if (prcNewWindow) {
        SetWindowPos(hwnd, nullptr, 
            prcNewWindow->left, prcNewWindow->top,
            prcNewWindow->right - prcNewWindow->left,
            prcNewWindow->bottom - prcNewWindow->top,
            SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

RECT DPIHelper::ScaleRect(const RECT& rc, float fromDpi, float toDpi) {
    float scale = toDpi / fromDpi;
    RECT result;
    result.left = static_cast<LONG>(rc.left * scale);
    result.top = static_cast<LONG>(rc.top * scale);
    result.right = static_cast<LONG>(rc.right * scale);
    result.bottom = static_cast<LONG>(rc.bottom * scale);
    return result;
}
