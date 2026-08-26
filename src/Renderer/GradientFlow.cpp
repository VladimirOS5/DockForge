#include "GradientFlow.h"
#include <cmath>

void GradientFlow::Update(float deltaTime) {
    m_offset += deltaTime * 0.15f;
    if (m_offset > 1.0f) m_offset -= 1.0f;
}

void GradientFlow::Render(ID2D1RenderTarget* rt, float x, float y, float w, float h) {
    Microsoft::WRL::ComPtr<ID2D1LinearGradientBrush> brush;
    Microsoft::WRL::ComPtr<ID2D1GradientStopCollection> stops;
    D2D1_GRADIENT_STOP gs[3];
    float o = m_offset;
    gs[0] = { 0.0f, {0.1f + 0.1f * sinf(o * 6.28f), 0.2f + 0.1f * cosf(o * 4.0f), 0.4f + 0.2f * sinf(o * 3.0f), 0.6f} };
    gs[1] = { 0.5f, {0.3f + 0.2f * cosf(o * 5.0f), 0.1f + 0.1f * sinf(o * 7.0f), 0.5f + 0.2f * cosf(o * 2.0f), 0.5f} };
    gs[2] = { 1.0f, {0.5f + 0.2f * sinf(o * 3.0f), 0.2f + 0.2f * cosf(o * 6.0f), 0.3f + 0.1f * sinf(o * 4.0f), 0.6f} };
    rt->CreateGradientStopCollection(gs, 3, &stops);
    if (!stops) return;
    rt->CreateLinearGradientBrush(
        D2D1::LinearGradientBrushProperties(D2D1::Point2F(x, y), D2D1::Point2F(x + w, y + h)),
        stops.Get(), &brush);
    if (brush) rt->FillRectangle(D2D1::RectF(x, y, x + w, y + h), brush.Get());
}