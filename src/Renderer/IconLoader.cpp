#include "IconLoader.h"
#include "../Utils/Logger.h"
#include <shellapi.h>

IconLoader::IconLoader() {}
IconLoader::IconLoader(ID2D1RenderTarget* renderTarget) : m_renderTarget(renderTarget) {}
IconLoader::~IconLoader() { Shutdown(); }

bool IconLoader::Initialize(ID2D1RenderTarget* rt) {
    if (rt) m_renderTarget = rt;
    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&m_wicFactory));
    if (FAILED(hr)) { LOG_ERROR("Failed to create WIC factory"); return false; }
    return true;
}
void IconLoader::Shutdown() { m_wicFactory.Reset(); m_renderTarget = nullptr; }

LoadedIcon IconLoader::LoadSystemIcon(const std::wstring& exePath, int size) {
    LoadedIcon icon; HICON hIcon = nullptr;
    ExtractIconExW(exePath.c_str(), 0, &hIcon, nullptr, 1);
    if (hIcon) { icon = HIconToLoadedIcon(hIcon, size); DestroyIcon(hIcon); }
    return icon;
}
LoadedIcon IconLoader::LoadFileIcon(const std::wstring& filePath, int size) {
    LoadedIcon icon; SHFILEINFOW sfi = {};
    if (SHGetFileInfoW(filePath.c_str(), 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_LARGEICON)) {
        icon = HIconToLoadedIcon(sfi.hIcon, size); DestroyIcon(sfi.hIcon);
    }
    return icon;
}
LoadedIcon IconLoader::LoadUWPAppIcon(const std::wstring&, int) { return LoadedIcon(); }
LoadedIcon IconLoader::LoadFromExe(const std::wstring& exePath, int size) { return LoadSystemIcon(exePath, size); }
LoadedIcon IconLoader::LoadFromHICON(HICON hIcon, int size) { return HIconToLoadedIcon(hIcon, size); }
bool IconLoader::ExtractIconFrames(const std::wstring&, int, LoadedIcon&) { return false; }
void IconLoader::SetRenderTarget(ID2D1RenderTarget* renderTarget) { m_renderTarget = renderTarget; }

LoadedIcon IconLoader::HIconToLoadedIcon(HICON hIcon, int) {
    LoadedIcon result;
    if (!m_wicFactory || !m_renderTarget) return result;
    ICONINFO ii = {}; if (!GetIconInfo(hIcon, &ii)) return result;
    BITMAP bm = {}; GetObjectW(ii.hbmColor, sizeof(bm), &bm);
    Microsoft::WRL::ComPtr<IWICBitmap> wicBitmap;
    HRESULT hr = m_wicFactory->CreateBitmapFromHBITMAP(ii.hbmColor, nullptr, WICBitmapUsePremultipliedAlpha, &wicBitmap);
    DeleteObject(ii.hbmMask); DeleteObject(ii.hbmColor);
    if (FAILED(hr)) return result;
    Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
    hr = m_wicFactory->CreateFormatConverter(&converter);
    if (FAILED(hr)) return result;
    hr = converter->Initialize(wicBitmap.Get(), GUID_WICPixelFormat32bppPBGRA,
        WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeMedianCut);
    if (FAILED(hr)) return result;
    Microsoft::WRL::ComPtr<ID2D1Bitmap> d2dBitmap;
    hr = m_renderTarget->CreateBitmapFromWicBitmap(converter.Get(), &d2dBitmap);
    if (SUCCEEDED(hr) && d2dBitmap) {
        result.frames.push_back(d2dBitmap); result.frameCount = 1;
        D2D1_SIZE_F sz = d2dBitmap->GetSize();
        result.width = static_cast<int>(sz.width); result.height = static_cast<int>(sz.height);
    }
    return result;
}