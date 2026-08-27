#include "GradientFlow.h"
#include <cmath>

void GradientFlow::Update(float deltaTime, float audioLevel, float bassLevel, float midLevel) {
    m_time += deltaTime;
    // Speed increases with audio
    m_offset += deltaTime * (0.1f + audioLevel * 0.3f);
    if (m_offset > 1.0f) m_offset -= 1.0f;
    
    UpdateGradientStops(audioLevel, bassLevel, midLevel);
}

void GradientFlow::UpdateGradientStops(float audioLevel, float bassLevel, float midLevel) {
    // Only recalculate if audio changed significantly (optimization)
    float delta = std::abs(audioLevel - m_lastAudio) + std::abs(bassLevel - m_lastBass) + 
                  std::abs(midLevel - m_lastMid);
    if (delta < 0.02f && m_lastAudio >= 0) return;
    
    m_lastAudio = audioLevel;
    m_lastBass = bassLevel;
    m_lastMid = midLevel;
    
    float o = m_offset;
    float i = m_intensity;
    
    // Audio-reactive colors: bass affects warm tones, mid affects cool tones
    m_stops[0] = { 0.0f, {
        0.05f + 0.15f * bassLevel * i,
        0.05f + 0.1f * midLevel * i,
        0.15f + 0.2f * audioLevel * i,
        0.5f + 0.2f * audioLevel
    }};
    m_stops[1] = { 0.25f, {
        0.1f + 0.3f * bassLevel * i + 0.1f * sinf(o * 6.28f),
        0.08f + 0.15f * midLevel * i + 0.08f * cosf(o * 4.0f),
        0.25f + 0.25f * audioLevel * i + 0.15f * sinf(o * 3.0f),
        0.45f + 0.15f * audioLevel
    }};
    m_stops[2] = { 0.5f, {
        0.2f + 0.3f * bassLevel * i + 0.15f * cosf(o * 5.0f),
        0.1f + 0.2f * midLevel * i + 0.1f * sinf(o * 7.0f),
        0.35f + 0.3f * audioLevel * i + 0.2f * cosf(o * 2.0f),
        0.4f + 0.15f * audioLevel
    }};
    m_stops[3] = { 0.75f, {
        0.15f + 0.25f * bassLevel * i + 0.12f * sinf(o * 3.0f),
        0.12f + 0.18f * midLevel * i + 0.15f * cosf(o * 6.0f),
        0.3f + 0.25f * audioLevel * i + 0.1f * sinf(o * 4.0f),
        0.45f + 0.1f * audioLevel
    }};
    m_stops[4] = { 1.0f, {
        0.08f + 0.2f * bassLevel * i,
        0.06f + 0.12f * midLevel * i,
        0.2f + 0.2f * audioLevel * i,
        0.5f + 0.2f * audioLevel
    }};
    
    // Invalidate brushes
    m_stopCollection.Reset();
    m_linearBrush.Reset();
    m_radialBrush.Reset();
}

void GradientFlow::CreateBrushes(ID2D1RenderTarget* rt, float x, float y, float w, float h, GradientDirection dir) {
    if (!m_stopCollection) {
        rt->CreateGradientStopCollection(m_stops, 5, &m_stopCollection);
    }
    if (!m_stopCollection) return;
    
    if (dir == GradientDirection::Radial) {
        if (!m_radialBrush) {
            rt->CreateRadialGradientBrush(
                D2D1::RadialGradientBrushProperties(
                    D2D1::Point2F(x + w/2, y + h/2), D2D1::Point2F(0, 0), w/2, h/2),
                m_stopCollection.Get(), &m_radialBrush);
        }
    } else {
        if (!m_linearBrush) {
            D2D1_POINT_2F start, end;
            if (dir == GradientDirection::Horizontal) {
                start = D2D1::Point2F(x, y);
                end = D2D1::Point2F(x + w, y + h);
            } else {
                start = D2D1::Point2F(x + w/2, y + h);
                end = D2D1::Point2F(x + w/2, y);
            }
            rt->CreateLinearGradientBrush(
                D2D1::LinearGradientBrushProperties(start, end),
                m_stopCollection.Get(), &m_linearBrush);
        }
    }
}

void GradientFlow::Render(ID2D1RenderTarget* rt, float x, float y, float w, float h, GradientDirection dir) {
    CreateBrushes(rt, x, y, w, h, dir);
    
    if (dir == GradientDirection::Radial && m_radialBrush) {
        rt->FillRectangle(D2D1::RectF(x, y, x + w, y + h), m_radialBrush.Get());
    } else if (m_linearBrush) {
        rt->FillRectangle(D2D1::RectF(x, y, x + w, y + h), m_linearBrush.Get());
    }
}