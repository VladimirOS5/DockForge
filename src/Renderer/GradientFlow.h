#pragma once
#include <d2d1.h>
#include <wrl/client.h>
#include <string>

enum class GradientDirection { Horizontal, Vertical, Radial };

class GradientFlow {
public:
    void Update(float deltaTime, float audioLevel, float bassLevel, float midLevel);
    void Render(ID2D1RenderTarget* rt, float x, float y, float w, float h, 
                GradientDirection dir = GradientDirection::Horizontal);
    void SetColorIntensity(float intensity) { m_intensity = intensity; }
private:
    void UpdateGradientStops(float audioLevel, float bassLevel, float midLevel);
    void CreateBrushes(ID2D1RenderTarget* rt, float x, float y, float w, float h, GradientDirection dir);
    
    float m_offset = 0.0f;
    float m_intensity = 1.0f;
    float m_time = 0.0f;
    
    // Cached stops
    float m_lastAudio = -1.0f;
    float m_lastBass = -1.0f;
    float m_lastMid = -1.0f;
    D2D1_GRADIENT_STOP m_stops[5];
    
    Microsoft::WRL::ComPtr<ID2D1GradientStopCollection> m_stopCollection;
    Microsoft::WRL::ComPtr<ID2D1LinearGradientBrush> m_linearBrush;
    Microsoft::WRL::ComPtr<ID2D1RadialGradientBrush> m_radialBrush;
};