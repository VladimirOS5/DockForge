#include "TextureAtlas.h"
#include "../Utils/Logger.h"

bool TextureAtlas::Initialize(ID2D1RenderTarget* rt, int maxSize) {
    m_renderTarget = rt;
    m_atlasSize = maxSize;
    m_currentX = 0; m_currentY = 0; m_rowHeight = 0;
    m_dirty = false;
    return RebuildAtlas();
}

bool TextureAtlas::RebuildAtlas() {
    if (!m_renderTarget) return false;

    D2D1_BITMAP_PROPERTIES props = {};
    props.pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED);
    props.dpiX = 96.0f; props.dpiY = 96.0f;

    HRESULT hr = m_renderTarget->CreateBitmap(D2D1::SizeU(m_atlasSize, m_atlasSize),
        nullptr, 0, &props, m_atlasBitmap.GetAddressOf());
    if (FAILED(hr)) { LOG_ERROR("Failed to create atlas bitmap"); return false; }

    // Clear to transparent
    Microsoft::WRL::ComPtr<ID2D1BitmapRenderTarget> rt;
    hr = m_renderTarget->CreateCompatibleRenderTarget(
        D2D1::SizeF(static_cast<float>(m_atlasSize), static_cast<float>(m_atlasSize)), rt.GetAddressOf());
    if (SUCCEEDED(hr) && rt) {
        rt->BeginDraw();
        rt->Clear(D2D1::ColorF(0, 0, 0, 0));
        rt->EndDraw();
        Microsoft::WRL::ComPtr<ID2D1Bitmap> bmp;
        rt->GetBitmap(bmp.GetAddressOf());
        if (bmp) {
            D2D1_POINT_2U destPoint = { 0, 0 };
            D2D1_RECT_U srcRect = { 0, 0, static_cast<UINT32>(m_atlasSize), static_cast<UINT32>(m_atlasSize) };
            m_atlasBitmap->CopyFromBitmap(&destPoint, bmp.Get(), &srcRect);
        }
    }

    m_currentX = 0; m_currentY = 0; m_rowHeight = 0;
    m_entries.clear();
    m_dirty = false;
    LOG_INFO("Texture atlas rebuilt: " + std::to_string(m_atlasSize) + "x" + std::to_string(m_atlasSize));
    return true;
}

bool TextureAtlas::AddBitmap(ID2D1Bitmap* bitmap, AtlasEntry& outEntry) {
    if (!bitmap || !m_atlasBitmap) return false;

    D2D1_SIZE_F sz = bitmap->GetSize();
    int w = static_cast<int>(sz.width);
    int h = static_cast<int>(sz.height);
    if (w > m_atlasSize || h > m_atlasSize) return false;

    // Simple shelf packing
    if (m_currentX + w > m_atlasSize) {
        m_currentX = 0;
        m_currentY += m_rowHeight + 2; // 2px padding
        m_rowHeight = 0;
    }
    if (m_currentY + h > m_atlasSize) {
        LOG_WARN("Atlas full! Rebuilding with larger size or clearing.");
        // For now, just clear and start over
        Clear();
        return AddBitmap(bitmap, outEntry);
    }

    // Copy bitmap into atlas
    D2D1_POINT_2U destPoint = { static_cast<UINT32>(m_currentX), static_cast<UINT32>(m_currentY) };
    D2D1_RECT_U srcRect = { 0, 0, static_cast<UINT32>(w), static_cast<UINT32>(h) };
    HRESULT hr = m_atlasBitmap->CopyFromBitmap(&destPoint, bitmap, &srcRect);
    if (FAILED(hr)) { LOG_ERROR("Failed to copy bitmap into atlas"); return false; }

    outEntry.x = m_currentX;
    outEntry.y = m_currentY;
    outEntry.width = w;
    outEntry.height = h;
    outEntry.originalWidth = w;
    outEntry.originalHeight = h;
    outEntry.u0 = static_cast<float>(m_currentX) / m_atlasSize;
    outEntry.v0 = static_cast<float>(m_currentY) / m_atlasSize;
    outEntry.u1 = static_cast<float>(m_currentX + w) / m_atlasSize;
    outEntry.v1 = static_cast<float>(m_currentY + h) / m_atlasSize;

    m_currentX += w + 2; // 2px padding
    if (h > m_rowHeight) m_rowHeight = h;

    m_entries.push_back(outEntry);
    return true;
}

void TextureAtlas::Clear() {
    m_entries.clear();
    m_pendingBitmaps.clear();
    RebuildAtlas();
}

bool TextureAtlas::Pack() {
    // Already packed incrementally
    return true;
}
