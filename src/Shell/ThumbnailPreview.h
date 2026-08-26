#pragma once
#include <windows.h>
#include <dwmapi.h>
#include <wrl/client.h>

class ThumbnailPreview {
public:
    ThumbnailPreview();
    ~ThumbnailPreview();
    
    bool Show(HWND sourceHwnd, HWND destHwnd, const RECT& destRect);
    void Hide();
    void UpdatePosition(const RECT& destRect);
    bool IsVisible() const { return m_visible; }
    HWND GetSourceHwnd() const { return m_sourceHwnd; }
    
private:
    HTHUMBNAIL m_thumbnail = nullptr;
    HWND m_sourceHwnd = nullptr;
    HWND m_destHwnd = nullptr;
    bool m_visible = false;
};