#pragma once
#include <string>
#include <vector>
#include <wrl/client.h>
#include <d2d1.h>
#include <wincodec.h>
#include "TextureAtlas.h"

struct LoadedIcon {
    Microsoft::WRL::ComPtr<ID2D1Bitmap> bitmap;
    int width = 0;
    int height = 0;
    int frameCount = 1;
    int currentFrame = 0;
    int frameDelayMs = 100;
    std::vector<Microsoft::WRL::ComPtr<ID2D1Bitmap>> frames;
};

class IconLoader {
public:
    IconLoader(ID2D1RenderTarget* renderTarget);
    ~IconLoader();
    bool Initialize();
    void Shutdown();
    LoadedIcon LoadSystemIcon(const std::wstring& iconPath, int size);
    LoadedIcon LoadFileIcon(const std::wstring& filePath, int size);
    LoadedIcon LoadUWPAppIcon(const std::wstring& appUserModelId, int size);
    LoadedIcon LoadFromHICON(HICON hIcon, int size);
    std::vector<LoadedIcon> ExtractIconFrames(const std::wstring& filePath);
    void SetRenderTarget(ID2D1RenderTarget* renderTarget);
private:
    Microsoft::WRL::ComPtr<IWICImagingFactory> m_wicFactory;
    ID2D1RenderTarget* m_renderTarget = nullptr;
    LoadedIcon HIconToLoadedIcon(HICON hIcon, int size);
};
