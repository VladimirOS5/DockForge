#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <d2d1.h>
#include <wrl/client.h>

struct LoadedIcon {
    std::wstring path;
    std::wstring displayName;
    bool isAnimated = false;
    int frameCount = 1;
    int frameDelayMs = 100;
    std::vector<Microsoft::WRL::ComPtr<ID2D1Bitmap>> frames;
    // For texture atlas
    float u0 = 0, v0 = 0, u1 = 0, v1 = 0;
    int atlasX = 0, atlasY = 0;
    int width = 0, height = 0;
};

class IconLoader {
public:
    bool Initialize(ID2D1RenderTarget* rt);
    LoadedIcon LoadFromExe(const std::wstring& exePath, int size = 48);
    LoadedIcon LoadFromFile(const std::wstring& filePath, int size = 48);
    LoadedIcon LoadAnimated(const std::wstring& filePath, int size = 48);
    LoadedIcon LoadSystemIcon(const std::wstring& shellPath, int size = 48);
    void ReleaseIcon(LoadedIcon& icon);
private:
    Microsoft::WRL::ComPtr<ID2D1RenderTarget> m_renderTarget;
    Microsoft::WRL::ComPtr<IWICImagingFactory> m_wicFactory;
    bool m_initialized = false;

    Microsoft::WRL::ComPtr<ID2D1Bitmap> DecodeToBitmap(const std::wstring& path, int targetSize);
    bool DecodeAnimated(const std::wstring& path, int targetSize, LoadedIcon& outIcon);
    Microsoft::WRL::ComPtr<ID2D1Bitmap> CreateBitmapFromHICON(HICON hIcon, int size);
    Microsoft::WRL::ComPtr<ID2D1Bitmap> ScaleBitmap(ID2D1Bitmap* source, int targetSize);
};
