#pragma once
#include <windows.h>
#include <d2d1.h>
#include <d2d1_1.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <vector>
#include <string>

struct LoadedIcon {
    std::vector<Microsoft::WRL::ComPtr<ID2D1Bitmap>> frames;
    int frameCount = 0;
    int currentFrame = 0;
    float frameTime = 0.0f;
    float frameDuration = 100.0f;
    bool isAnimated = false;
};

struct AtlasEntry {
    float u0 = 0, v0 = 0, u1 = 0, v1 = 0;
    int width = 0, height = 0;
};

class IconLoader {
public:
    IconLoader();
    bool Initialize(ID2D1RenderTarget* rt);
    LoadedIcon LoadFromExe(const std::wstring& exePath, int size);
    LoadedIcon LoadSystemIcon(const std::wstring& path, int size);
    LoadedIcon LoadFromHICON(HICON hIcon, int size);
private:
    ID2D1RenderTarget* m_renderTarget = nullptr;
    Microsoft::WRL::ComPtr<IWICImagingFactory> m_wicFactory;
};
