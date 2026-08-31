#pragma once
#include <string>
#include <vector>
#include <wrl/client.h>
#include <d2d1.h>
#include <wincodec.h>
#include "TextureAtlas.h"

struct LoadedIcon {
    Microsoft::WRL::ComPtr<ID2D1Bitmap> bitmap;
    int width = 0, height = 0;
    int frameCount = 1, currentFrame = 0, frameDelayMs = 100;
    bool isAnimated = false;
    int frameDuration = 100;
    std::vector<Microsoft::WRL::ComPtr<ID2D1Bitmap>> frames;
};

class IconLoader {
public:
    IconLoader();
    IconLoader(ID2D1RenderTarget* renderTarget);
    ~IconLoader();
    bool Initialize(ID2D1RenderTarget* rt = nullptr);
    void Shutdown();
    LoadedIcon LoadSystemIcon(const std::wstring& iconPath, int size);
    LoadedIcon LoadFileIcon(const std::wstring& filePath, int size);
    LoadedIcon LoadUWPAppIcon(const std::wstring& appUserModelId, int size);
    LoadedIcon LoadFromHICON(HICON hIcon, int size);
    LoadedIcon LoadFromExe(const std::wstring& exePath, int size);
    bool ExtractIconFrames(const std::wstring& path, int size, LoadedIcon& out);
    void SetRenderTarget(ID2D1RenderTarget* renderTarget);
private:
    Microsoft::WRL::ComPtr<IWICImagingFactory> m_wicFactory;
    ID2D1RenderTarget* m_renderTarget = nullptr;
    LoadedIcon HIconToLoadedIcon(HICON hIcon, int size);
};