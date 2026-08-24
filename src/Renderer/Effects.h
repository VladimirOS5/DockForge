#pragma once
#include <d2d1_1.h>
#include <d2d1effects.h>
#include <wrl/client.h>
#include <vector>

// Gaussian Blur wrapper
class BlurEffect {
public:
    bool Initialize(ID2D1DeviceContext* dc);
    ID2D1Image* Apply(ID2D1Image* input, float radius);
private:
    Microsoft::WRL::ComPtr<ID2D1Effect> m_effect;
};

// Acrylic: blur + noise + tint
class AcrylicEffect {
public:
    bool Initialize(ID2D1DeviceContext* dc, UINT width, UINT height);
    ID2D1Image* Apply(ID2D1Image* input);
    void SetTint(float r, float g, float b, float opacity);
    void SetBlurRadius(float radius);
private:
    Microsoft::WRL::ComPtr<ID2D1DeviceContext> m_dc;
    Microsoft::WRL::ComPtr<ID2D1Effect> m_blur;
    Microsoft::WRL::ComPtr<ID2D1Effect> m_blend;
    Microsoft::WRL::ComPtr<ID2D1Effect> m_colorMatrix;
    Microsoft::WRL::ComPtr<ID2D1Bitmap> m_noiseBitmap;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_tintBrush;
    bool CreateNoiseBitmap(UINT width, UINT height);
};

// Liquid Glass: blur + displacement + color matrix + specular
class LiquidGlassEffect {
public:
    bool Initialize(ID2D1DeviceContext* dc, UINT width, UINT height);
    ID2D1Image* Apply(ID2D1Image* input);
    void SetRefractionStrength(float strength);
    void SetGlowIntensity(float intensity);
    void SetSaturation(float sat);
    void UpdateAnimation(float deltaTime);
private:
    Microsoft::WRL::ComPtr<ID2D1DeviceContext> m_dc;
    Microsoft::WRL::ComPtr<ID2D1Effect> m_blur;
    Microsoft::WRL::ComPtr<ID2D1Effect> m_displacement;
    Microsoft::WRL::ComPtr<ID2D1Effect> m_colorMatrix;
    Microsoft::WRL::ComPtr<ID2D1Effect> m_flood;
    Microsoft::WRL::ComPtr<ID2D1Effect> m_composite;
    Microsoft::WRL::ComPtr<ID2D1Bitmap> m_displacementBitmap;
    float m_time = 0.0f;
    float m_refractionStrength = 3.0f;
    float m_glowIntensity = 0.35f;
    float m_saturation = 1.2f;
    bool CreateDisplacementBitmap(UINT width, UINT height);
};
