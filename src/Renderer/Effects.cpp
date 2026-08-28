#include "Effects.h"
#include "../Utils/Logger.h"
#include <cmath>

// ==================== BlurEffect ====================
bool BlurEffect::Initialize(ID2D1DeviceContext* dc) {
    if (!dc) return false;
    HRESULT hr = dc->CreateEffect(CLSID_D2D1GaussianBlur, m_effect.GetAddressOf());
    if (FAILED(hr)) { LOG_ERROR("Failed to create GaussianBlur effect"); return false; }
    m_effect->SetValue(D2D1_GAUSSIANBLUR_PROP_BORDER_MODE, D2D1_BORDER_MODE_HARD);
    return true;
}

ID2D1Image* BlurEffect::Apply(ID2D1Image* input, float radius) {
    if (!m_effect || !input) return input;
    m_effect->SetInput(0, input);
    m_effect->SetValue(D2D1_GAUSSIANBLUR_PROP_STANDARD_DEVIATION, radius);
    ID2D1Image* output = nullptr;
    m_effect->GetOutput(&output);
    return output;
}

// ==================== AcrylicEffect ====================
bool AcrylicEffect::Initialize(ID2D1DeviceContext* dc, UINT width, UINT height) {
    if (!dc) return false;
    m_dc = dc;

    HRESULT hr = dc->CreateEffect(CLSID_D2D1GaussianBlur, m_blur.GetAddressOf());
    if (FAILED(hr)) return false;
    m_blur->SetValue(D2D1_GAUSSIANBLUR_PROP_BORDER_MODE, D2D1_BORDER_MODE_HARD);

    hr = dc->CreateEffect(CLSID_D2D1ColorMatrix, m_colorMatrix.GetAddressOf());
    if (FAILED(hr)) return false;

    hr = dc->CreateEffect(CLSID_D2D1Blend, m_blend.GetAddressOf());
    if (FAILED(hr)) return false;
    m_blend->SetValue(D2D1_BLEND_PROP_MODE, D2D1_BLEND_MODE_MULTIPLY);

    if (!CreateNoiseBitmap(width, height)) {
        LOG_WARN("Failed to create noise bitmap, acrylic will work without noise");
    }

    hr = dc->CreateSolidColorBrush(D2D1::ColorF(0.12f, 0.12f, 0.14f, 0.6f), m_tintBrush.GetAddressOf());
    if (FAILED(hr)) return false;

    return true;
}

bool AcrylicEffect::CreateNoiseBitmap(UINT width, UINT height) {
    if (!m_dc) return false;
    std::vector<uint8_t> pixels(width * height * 4);
    for (UINT y = 0; y < height; ++y) {
        for (UINT x = 0; x < width; ++x) {
            uint8_t v = static_cast<uint8_t>((rand() % 40) + 100);
            UINT idx = (y * width + x) * 4;
            pixels[idx] = v;
            pixels[idx+1] = v;
            pixels[idx+2] = v;
            pixels[idx+3] = 15;
        }
    }
    D2D1_BITMAP_PROPERTIES props = {};
    props.pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED);
    props.dpiX = 96.0f; props.dpiY = 96.0f;
    HRESULT hr = m_dc->CreateBitmap(D2D1::SizeU(width, height), pixels.data(), width * 4, &props, m_noiseBitmap.GetAddressOf());
    return SUCCEEDED(hr);
}

ID2D1Image* AcrylicEffect::Apply(ID2D1Image* input) {
    // FIX: Added nullptr checks
    if (!m_blur || !m_colorMatrix || !input) return input;

    m_blur->SetInput(0, input);
    ID2D1Image* blurred = nullptr;
    m_blur->GetOutput(&blurred);

    D2D1_MATRIX_5X4_F matrix = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, m_tintBrush ? m_tintBrush->GetColor().a : 0.6f,
        m_tintBrush ? m_tintBrush->GetColor().r * 0.3f : 0.0f,
        m_tintBrush ? m_tintBrush->GetColor().g * 0.3f : 0.0f,
        m_tintBrush ? m_tintBrush->GetColor().b * 0.3f : 0.0f,
        0.0f
    };
    m_colorMatrix->SetInputEffect(0, m_blur.Get());
    m_colorMatrix->SetValue(D2D1_COLORMATRIX_PROP_COLOR_MATRIX, matrix);

    ID2D1Image* tinted = nullptr;
    m_colorMatrix->GetOutput(&tinted);
    return tinted;
}

void AcrylicEffect::SetTint(float r, float g, float b, float opacity) {
    if (m_tintBrush) {
        m_tintBrush->SetColor(D2D1::ColorF(r, g, b, opacity));
    }
}

void AcrylicEffect::SetBlurRadius(float radius) {
    if (m_blur) {
        m_blur->SetValue(D2D1_GAUSSIANBLUR_PROP_STANDARD_DEVIATION, radius);
    }
}

// ==================== LiquidGlassEffect ====================
bool LiquidGlassEffect::Initialize(ID2D1DeviceContext* dc, UINT width, UINT height) {
    if (!dc) return false;
    m_dc = dc;

    HRESULT hr = dc->CreateEffect(CLSID_D2D1GaussianBlur, m_blur.GetAddressOf());
    if (FAILED(hr)) return false;
    m_blur->SetValue(D2D1_GAUSSIANBLUR_PROP_BORDER_MODE, D2D1_BORDER_MODE_HARD);
    m_blur->SetValue(D2D1_GAUSSIANBLUR_PROP_STANDARD_DEVIATION, 20.0f);

    hr = dc->CreateEffect(CLSID_D2D1DisplacementMap, m_displacement.GetAddressOf());
    if (FAILED(hr)) return false;
    m_displacement->SetValue(D2D1_DISPLACEMENTMAP_PROP_SCALE, m_refractionStrength);
    m_displacement->SetValue(D2D1_DISPLACEMENTMAP_PROP_X_CHANNEL_SELECT, D2D1_CHANNEL_SELECTOR_R);
    m_displacement->SetValue(D2D1_DISPLACEMENTMAP_PROP_Y_CHANNEL_SELECT, D2D1_CHANNEL_SELECTOR_G);

    hr = dc->CreateEffect(CLSID_D2D1Saturation, m_colorMatrix.GetAddressOf());
    if (FAILED(hr)) return false;
    m_colorMatrix->SetValue(D2D1_SATURATION_PROP_SATURATION, m_saturation);

    if (!CreateDisplacementBitmap(width, height)) {
        LOG_WARN("Failed to create displacement bitmap");
    }
    return true;
}

bool LiquidGlassEffect::CreateDisplacementBitmap(UINT width, UINT height) {
    if (!m_dc) return false;
    std::vector<uint8_t> pixels(width * height * 4);
    for (UINT y = 0; y < height; ++y) {
        for (UINT x = 0; x < width; ++x) {
            float nx = static_cast<float>(x) / width;
            float ny = static_cast<float>(y) / height;
            float dx = std::sin(nx * 20.0f + m_time) * 0.5f + 0.5f;
            float dy = std::cos(ny * 20.0f + m_time * 0.7f) * 0.5f + 0.5f;
            UINT idx = (y * width + x) * 4;
            pixels[idx] = static_cast<uint8_t>(dx * 255);
            pixels[idx+1] = static_cast<uint8_t>(dy * 255);
            pixels[idx+2] = 128;
            pixels[idx+3] = 255;
        }
    }
    D2D1_BITMAP_PROPERTIES props = {};
    props.pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED);
    props.dpiX = 96.0f; props.dpiY = 96.0f;
    HRESULT hr = m_dc->CreateBitmap(D2D1::SizeU(width, height), pixels.data(), width * 4, &props, m_displacementBitmap.GetAddressOf());
    return SUCCEEDED(hr);
}

ID2D1Image* LiquidGlassEffect::Apply(ID2D1Image* input) {
    if (!m_blur || !m_displacement || !m_colorMatrix || !input) return input;

    m_blur->SetInput(0, input);
    m_displacement->SetInputEffect(0, m_blur.Get());
    if (m_displacementBitmap) {
        m_displacement->SetInput(1, m_displacementBitmap.Get());
    }
    m_colorMatrix->SetInputEffect(0, m_displacement.Get());

    ID2D1Image* output = nullptr;
    m_colorMatrix->GetOutput(&output);
    return output;
}

void LiquidGlassEffect::SetRefractionStrength(float strength) {
    m_refractionStrength = strength;
    if (m_displacement) {
        m_displacement->SetValue(D2D1_DISPLACEMENTMAP_PROP_SCALE, strength);
    }
}

void LiquidGlassEffect::SetGlowIntensity(float intensity) {
    m_glowIntensity = intensity;
}

void LiquidGlassEffect::SetSaturation(float sat) {
    m_saturation = sat;
    if (m_colorMatrix) {
        m_colorMatrix->SetValue(D2D1_SATURATION_PROP_SATURATION, sat);
    }
}

void LiquidGlassEffect::UpdateAnimation(float deltaTime) {
    m_time += deltaTime * 2.0f;
    if (m_displacementBitmap && m_dc) {
        D2D1_SIZE_U size = m_displacementBitmap->GetPixelSize();
        CreateDisplacementBitmap(size.width, size.height);
        if (m_displacement) {
            m_displacement->SetInput(1, m_displacementBitmap.Get());
        }
    }
}
