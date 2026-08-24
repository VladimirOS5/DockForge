#include "EffectRenderer.h"
#include "../Utils/Logger.h"

EffectRenderer::EffectRenderer() {}
EffectRenderer::~EffectRenderer() {}

bool EffectRenderer::Initialize() {
    D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1 };
    HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT, levels, ARRAYSIZE(levels),
        D3D11_SDK_VERSION, m_d3dDevice.GetAddressOf(), nullptr, nullptr);
    if (FAILED(hr)) { LOG_ERROR("Failed to create WARP D3D11 device"); return false; }

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory1),
        nullptr, reinterpret_cast<void**>(m_factory1.GetAddressOf()));
    if (FAILED(hr)) { LOG_ERROR("Failed to create D2D1Factory1"); return false; }

    Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
    hr = m_d3dDevice.As(&dxgiDevice);
    if (FAILED(hr)) { LOG_ERROR("Failed to get IDXGIDevice"); return false; }

    hr = m_factory1->CreateDevice(dxgiDevice.Get(), m_d2dDevice.GetAddressOf());
    if (FAILED(hr)) { LOG_ERROR("Failed to create D2D device"); return false; }

    hr = m_d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, m_deviceContext.GetAddressOf());
    if (FAILED(hr)) { LOG_ERROR("Failed to create D2D device context"); return false; }

    LOG_INFO("EffectRenderer (WARP) initialized");
    return true;
}

bool EffectRenderer::CreateTargetBitmap(UINT width, UINT height) {
    if (!m_deviceContext) return false;
    D2D1_BITMAP_PROPERTIES1 props = {};
    props.pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED);
    props.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET;
    HRESULT hr = m_deviceContext->CreateBitmap(D2D1::SizeU(width, height), nullptr, 0, &props, m_targetBitmap.GetAddressOf());
    if (FAILED(hr)) { LOG_ERROR("Failed to create effect target bitmap"); return false; }
    m_deviceContext->SetTarget(m_targetBitmap.Get());
    return true;
}

void EffectRenderer::Resize(UINT width, UINT height) {
    m_targetBitmap.Reset();
    CreateTargetBitmap(width, height);
}

void EffectRenderer::BeginDraw() {
    if (m_deviceContext) m_deviceContext->BeginDraw();
}

void EffectRenderer::EndDraw() {
    if (!m_deviceContext) return;
    HRESULT hr = m_deviceContext->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) {
        LOG_WARN("EffectRenderer target lost, reinitializing...");
        m_targetBitmap.Reset();
        // Bitmap will be recreated on next resize/draw
    }
}
