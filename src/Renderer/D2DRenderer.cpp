#include "D2DRenderer.h"
#include "../Utils/Logger.h"
#include <windows.h>

D2DRenderer::D2DRenderer() {}
D2DRenderer::~D2DRenderer() { DiscardDeviceResources(); }

bool D2DRenderer::Initialize(HWND hwnd) {
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, m_factory.GetAddressOf());
    if (FAILED(hr)) { LOG_ERROR("Failed to create D2D factory"); return false; }
    if (!CreateDeviceResources(hwnd)) return false;
    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(m_writeFactory.GetAddressOf()));
    if (SUCCEEDED(hr)) {
        m_writeFactory->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 14.0f, L"en-us", m_textFormat.GetAddressOf());
    }
    LOG_INFO("D2D renderer initialized");
    return true;
}

bool D2DRenderer::CreateDeviceResources(HWND hwnd) {
    RECT rc; GetClientRect(hwnd, &rc);
    D2D1_SIZE_U size = D2D1::SizeU(static_cast<UINT32>(rc.right - rc.left), static_cast<UINT32>(rc.bottom - rc.top));
    HRESULT hr = m_factory->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)),
        D2D1::HwndRenderTargetProperties(hwnd, size), m_renderTarget.GetAddressOf());
    if (FAILED(hr)) { LOG_ERROR("Failed to create HWND render target"); return false; }
    m_renderTarget->SetDpi(96.0f, 96.0f);
    return true;
}

void D2DRenderer::Resize(UINT width, UINT height) { if (m_renderTarget) m_renderTarget->Resize(D2D1::SizeU(width, height)); }
void D2DRenderer::BeginDraw() { if (m_renderTarget) m_renderTarget->BeginDraw(); }

void D2DRenderer::EndDraw() {
    if (!m_renderTarget) return;
    HRESULT hr = m_renderTarget->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) { LOG_WARN("D2D target lost"); DiscardDeviceResources(); }
}

void D2DRenderer::Clear(float r, float g, float b, float a) { if (m_renderTarget) m_renderTarget->Clear(D2D1::ColorF(r, g, b, a)); }

void D2DRenderer::FillRect(float x, float y, float w, float h, float r, float g, float b, float a) {
    if (!m_renderTarget) return;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
    if (SUCCEEDED(m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(r, g, b, a), brush.GetAddressOf())) && brush)
        m_renderTarget->FillRectangle(D2D1::RectF(x, y, x + w, y + h), brush.Get());
}

void D2DRenderer::DrawRoundedRect(float x, float y, float w, float h, float radius, float r, float g, float b, float a) {
    if (!m_renderTarget) return;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
    if (SUCCEEDED(m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(r, g, b, a), brush.GetAddressOf())) && brush) {
        D2D1_ROUNDED_RECT rr = { D2D1::RectF(x, y, x + w, y + h), radius, radius };
        m_renderTarget->FillRoundedRectangle(&rr, brush.Get());
    }
}

void D2DRenderer::DrawBitmap(ID2D1Bitmap* bitmap, float x, float y, float w, float h, float opacity) {
    if (!m_renderTarget || !bitmap) return;
    m_renderTarget->DrawBitmap(bitmap, D2D1::RectF(x, y, x + w, y + h), opacity, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, nullptr);
}

void D2DRenderer::DrawBitmapFromAtlas(ID2D1Bitmap* atlas, float destX, float destY, float destW, float destH,
    float u0, float v0, float u1, float v1, float opacity) {
    if (!m_renderTarget || !atlas) return;
    D2D1_SIZE_F atlasSize = atlas->GetSize();
    float srcX = u0 * atlasSize.width;
    float srcY = v0 * atlasSize.height;
    float srcW = (u1 - u0) * atlasSize.width;
    float srcH = (v1 - v0) * atlasSize.height;
    D2D1_RECT_F srcRect = { srcX, srcY, srcX + srcW, srcY + srcH };
    m_renderTarget->DrawBitmap(atlas, D2D1::RectF(destX, destY, destX + destW, destY + destH),
        opacity, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, &srcRect);
}

void D2DRenderer::DrawTextLayout(const std::wstring& text, float x, float y, float r, float g, float b, float size) {
    if (!m_renderTarget || !m_writeFactory) return;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> format;
    m_writeFactory->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, size, L"en-us", format.GetAddressOf());
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
    if (SUCCEEDED(m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(r, g, b), brush.GetAddressOf())) && brush && format)
        m_renderTarget->DrawTextW(text.c_str(), static_cast<UINT32>(text.length()), format.Get(),
            D2D1::RectF(x, y, x + 500, y + 50), brush.Get());
}

void D2DRenderer::DiscardDeviceResources() { m_renderTarget.Reset(); }
