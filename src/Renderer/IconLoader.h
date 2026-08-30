#pragma once
#include <d2d1.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <vector>
#include <string>
#include "TextureAtlas.h"

struct LoadedIcon {
    std::vector<Microsoft::WRL::ComPtr<ID2D1Bitmap>> frames;
    int frameCount = 1;
    int frameDuration = 100;
    int width = 0;
    int height = 0;
    bool isAnimated = false;
};

class IconLoader {
public:
    IconLoader();
    bool Initialize(ID2D1RenderTarget* rt);
    void Shutdown();
    LoadedIcon LoadSystemIcon(const std::wstring& exePath, int size);
    LoadedIcon LoadFileIcon(const std::wstring& filePath, int size);
    LoadedIcon LoadUWPAppIcon(const std::wstring& appUserModelId, int size);
    bool ExtractIconFrames(const std::wstring& path, int size, LoadedIcon& out);
private:
    ID2D1RenderTarget* m_renderTarget = nullptr;
    Microsoft::WRL::ComPtr<IWICImagingFactory> m_wicFactory;
};
