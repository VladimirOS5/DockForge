#pragma once
#include <d2d1_1.h>
#include <d3d11.h>
#include <wrl/client.h>

class EffectRenderer {
public:
    EffectRenderer();
    ~EffectRenderer();
    bool Initialize();
    void Resize(UINT width, UINT height);
    void BeginDraw();
    void EndDraw();
    ID2D1DeviceContext* GetContext() const { return m_deviceContext.Get(); }
    ID2D1Bitmap1* GetTargetBitmap() const { return m_targetBitmap.Get(); }
    bool IsInitialized() const { return m_deviceContext != nullptr; }
private:
    Microsoft::WRL::ComPtr<ID3D11Device> m_d3dDevice;
    Microsoft::WRL::ComPtr<ID2D1Factory1> m_factory1;
    Microsoft::WRL::ComPtr<ID2D1Device> m_d2dDevice;
    Microsoft::WRL::ComPtr<ID2D1DeviceContext> m_deviceContext;
    Microsoft::WRL::ComPtr<ID2D1Bitmap1> m_targetBitmap;
    bool CreateTargetBitmap(UINT width, UINT height);
};
