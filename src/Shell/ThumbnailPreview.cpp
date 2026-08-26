#include "ThumbnailPreview.h"
#include "../Utils/Logger.h"

ThumbnailPreview::ThumbnailPreview() = default;
ThumbnailPreview::~ThumbnailPreview() { Hide(); }

bool ThumbnailPreview::Show(HWND sourceHwnd, HWND destHwnd, const RECT& destRect) {
    if (m_visible && m_sourceHwnd == sourceHwnd && m_destHwnd == destHwnd) {
        UpdatePosition(destRect);
        return true;
    }
    
    Hide();
    
    if (!IsWindow(sourceHwnd) || !IsWindow(destHwnd)) return false;
    
    HRESULT hr = DwmRegisterThumbnail(destHwnd, sourceHwnd, &m_thumbnail);
    if (FAILED(hr)) {
        LOG_ERROR("DwmRegisterThumbnail failed: " + std::to_string(hr));
        return false;
    }
    
    m_sourceHwnd = sourceHwnd;
    m_destHwnd = destHwnd;
    m_visible = true;
    
    UpdatePosition(destRect);
    LOG_INFO("Thumbnail preview registered");
    return true;
}

void ThumbnailPreview::Hide() {
    if (m_thumbnail) {
        DwmUnregisterThumbnail(m_thumbnail);
        m_thumbnail = nullptr;
    }
    m_visible = false;
    m_sourceHwnd = nullptr;
    m_destHwnd = nullptr;
}

void ThumbnailPreview::UpdatePosition(const RECT& destRect) {
    if (!m_thumbnail || !m_visible) return;
    
    DWM_THUMBNAIL_PROPERTIES props = { sizeof(props) };
    props.dwFlags = DWM_TNP_RECTDESTINATION | DWM_TNP_OPACITY | DWM_TNP_VISIBLE | DWM_TNP_SOURCECLIENTAREAONLY;
    props.rcDestination = destRect;
    props.opacity = 255;
    props.fVisible = TRUE;
    props.fSourceClientAreaOnly = TRUE;
    
    HRESULT hr = DwmUpdateThumbnailProperties(m_thumbnail, &props);
    if (FAILED(hr)) {
        LOG_ERROR("DwmUpdateThumbnailProperties failed: " + std::to_string(hr));
    }
}