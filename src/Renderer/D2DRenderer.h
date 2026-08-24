#pragma once
#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>
#include <string>

class D2DRenderer {
public:
    D2DRenderer();
    ~D2DRenderer();
    bool Initialize(HWND hwnd);
    void Resize(UINT width, UINT height);
    void BeginDraw();
    void EndDraw();
    void Clear(float r, float g, float b, float a);
    void FillRect(float x, float y, float w, float h, float r, float g, float b, float a);
    void DrawRoundedRect(float x, float y, float w, float h, float radius, float r, float g, float b, float a);
    void DrawBitmap(ID2D1Bitmap* bitmap, float x, float y, float w, float h, float opacity);
    void DrawBitmapFromAtlas(ID2D1Bitmap* atlas, float destX, float destY, float destW, float destH,
        float u0, float v0, float u1, float v1, float opacity);
    void DrawTextLayout(const std::wstring& text, float x, float y, float r, float g, float b, float size);
    ID2D1HwndRenderTarget* GetRenderTarget() const { return m_renderTarget.Get(); }
    IDWriteFactory* GetWriteFactory() const { return m_writeFactory.Get(); }
    bool IsInitialized() const { return m_factory != nullptr; }
private:
    Microsoft::WRL::ComPtr<ID2D1Factory> m_factory;
    Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> m_renderTarget;
    Microsoft::WRL::ComPtr<IDWriteFactory> m_writeFactory;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> m_textFormat;
    bool CreateDeviceResources(HWND hwnd);
    void DiscardDeviceResources();
};
