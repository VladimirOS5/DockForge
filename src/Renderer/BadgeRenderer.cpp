#include "BadgeRenderer.h"
#include "../Utils/Logger.h"

bool BadgeRenderer::Initialize(ID2D1RenderTarget* rt, IDWriteFactory* writeFactory) {
    if (!rt || !writeFactory) return false;
    m_writeFactory = writeFactory;

    HRESULT hr = rt->CreateSolidColorBrush(D2D1::ColorF(0.95f, 0.2f, 0.2f, 1.0f), m_badgeBrush.GetAddressOf());
    if (FAILED(hr)) return false;

    hr = rt->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f), m_textBrush.GetAddressOf());
    if (FAILED(hr)) return false;

    hr = m_writeFactory->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_BOLD,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 10.0f, L"en-us", m_textFormat.GetAddressOf());
    if (FAILED(hr)) return false;

    m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    m_textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    m_initialized = true;
    return true;
}

void BadgeRenderer::DrawBadge(ID2D1RenderTarget* rt, float x, float y, int count, float iconSize) {
    if (!m_initialized || count <= 0) return;

    float badgeRadius = 8.0f;
    float badgeX = x + iconSize - badgeRadius - 2.0f;
    float badgeY = y - badgeRadius + 2.0f;

    // Draw red circle
    D2D1_ELLIPSE ellipse = { D2D1::Point2F(badgeX, badgeY), badgeRadius, badgeRadius };
    rt->FillEllipse(&ellipse, m_badgeBrush.Get());

    // Draw white border
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> borderBrush;
    rt->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.8f), borderBrush.GetAddressOf());
    if (borderBrush) rt->DrawEllipse(&ellipse, borderBrush.Get(), 1.5f);

    // Draw count text
    std::wstring text = (count > 99) ? L"99+" : std::to_wstring(count);
    float textW = badgeRadius * 2.0f;
    D2D1_RECT_F textRect = { badgeX - badgeRadius, badgeY - badgeRadius, badgeX + badgeRadius, badgeY + badgeRadius };
    rt->DrawTextW(text.c_str(), static_cast<UINT32>(text.length()), m_textFormat.Get(), &textRect, m_textBrush.Get());
}
