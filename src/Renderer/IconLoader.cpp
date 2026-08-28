#include "IconLoader.h"
#include "../Utils/Logger.h"
#include <shellapi.h>
#include <shlwapi.h>

IconLoader::IconLoader() {}

bool IconLoader::Initialize(ID2D1RenderTarget* rt) {
    m_renderTarget = rt;
    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
        IID_IWICImagingFactory, reinterpret_cast<LPVOID*>(&m_wicFactory));
    if (FAILED(hr)) { LOG_ERROR("Failed to create WIC factory"); return false; }
    return true;
}

LoadedIcon IconLoader::LoadFromExe(const std::wstring& exePath, int size) {
    LoadedIcon result;
    if (!m_renderTarget) return result;
    HICON hIcon = static_cast<HICON>(LoadImageW(nullptr, exePath.c_str(), IMAGE_ICON, size, size, LR_LOADFROMFILE));
    if (!hIcon) {
        WORD iconIndex = 0;
        ExtractIconExW(exePath.c_str(), iconIndex, &hIcon, nullptr, 1);
    }
    if (hIcon) {
        result = LoadFromHICON(hIcon, size);
        DestroyIcon(hIcon);
    }
    return result;
}

LoadedIcon IconLoader::LoadSystemIcon(const std::wstring& path, int size) {
    LoadedIcon result;
    if (!m_renderTarget) return result;
    SHFILEINFOW sfi = {};
    if (SHGetFileInfoW(path.c_str(), 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_LARGEICON)) {
        if (sfi.hIcon) {
            result = LoadFromHICON(sfi.hIcon, size);
            DestroyIcon(sfi.hIcon);
        }
    }
    return result;
}

LoadedIcon IconLoader::LoadFromHICON(HICON hIcon, int size) {
    LoadedIcon result;
    if (!m_renderTarget || !hIcon || !m_wicFactory) return result;

    // FIX: ICONINFO has no cbSize field!
    ICONINFO ii = {};
    if (!GetIconInfo(hIcon, &ii)) return result;

    Microsoft::WRL::ComPtr<IWICBitmap> wicBitmap;
    HRESULT hr = m_wicFactory->CreateBitmapFromHICON(hIcon, &wicBitmap);
    if (FAILED(hr) || !wicBitmap) {
        if (ii.hbmColor) DeleteObject(ii.hbmColor);
        if (ii.hbmMask) DeleteObject(ii.hbmMask);
        return result;
    }

    Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
    hr = m_wicFactory->CreateFormatConverter(&converter);
    if (SUCCEEDED(hr)) {
        hr = converter->Initialize(wicBitmap.Get(), GUID_WICPixelFormat32bppPBGRA,
            WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeMedianCut);
    }
    if (SUCCEEDED(hr) && converter) {
        Microsoft::WRL::ComPtr<ID2D1Bitmap> d2dBitmap;
        hr = m_renderTarget->CreateBitmapFromWicBitmap(converter.Get(), &d2dBitmap);
        if (SUCCEEDED(hr) && d2dBitmap) result.frames.push_back(d2dBitmap);
    }

    if (ii.hbmColor) DeleteObject(ii.hbmColor);
    if (ii.hbmMask) DeleteObject(ii.hbmMask);
    result.frameCount = static_cast<int>(result.frames.size());
    return result;
}
