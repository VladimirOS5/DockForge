#pragma once
#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>
#include <string>

class BadgeRenderer {
public:
    bool Initialize(ID2D1RenderTarget* rt, IDWriteFactory* writeFactory);
    void DrawBadge(ID2D1RenderTarget* rt, float x, float y, int count, float iconSize);
private:
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_badgeBrush;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_textBrush;
    Microsoft::WRL::ComPtr<IDWriteFactory> m_writeFactory;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> m_textFormat;
    bool m_initialized = false;
};
