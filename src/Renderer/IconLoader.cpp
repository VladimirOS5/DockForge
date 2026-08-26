#include "IconLoader.h"
#include "../Utils/Logger.h"
#include <wincodec.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <shobjidl.h>
#include <commoncontrols.h>

IconLoader::IconLoader() {}

bool IconLoader::Initialize(ID2D1RenderTarget* rt) {
    m_renderTarget = rt;
    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&m_wicFactory));
    if (FAILED(hr)) { LOG_ERROR("Failed to create WIC factory"); return false; }
    m_initialized = true;
    LOG_INFO("IconLoader initialized");
    return true;
}

Microsoft::WRL::ComPtr<ID2D1Bitmap> IconLoader::DecodeToBitmap(const std::wstring& path, int targetSize) {
    if (!m_wicFactory || !m_renderTarget) return nullptr;

    Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
    HRESULT hr = m_wicFactory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ,
        WICDecodeMetadataCacheOnDemand, decoder.GetAddressOf());
    if (FAILED(hr)) {
        // Try as icon resource
        HICON hIcon = ExtractIconW(GetModuleHandle(nullptr), path.c_str(), 0);
        if (hIcon) {
            auto bmp = CreateBitmapFromHICON(hIcon, targetSize);
            DestroyIcon(hIcon);
            return bmp;
        }
        return nullptr;
    }

    Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
    hr = decoder->GetFrame(0, frame.GetAddressOf());
    if (FAILED(hr)) return nullptr;

    Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
    hr = m_wicFactory->CreateFormatConverter(converter.GetAddressOf());
    if (FAILED(hr)) return nullptr;

    hr = converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppPBGRA,
        WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeMedianCut);
    if (FAILED(hr)) return nullptr;

    UINT w, h;
    converter->GetSize(&w, &h);

    // Scale to target size maintaining aspect ratio
    float scale = static_cast<float>(targetSize) / std::max(w, h);
    UINT newW = static_cast<UINT>(w * scale);
    UINT newH = static_cast<UINT>(h * scale);
    if (newW < 1) newW = 1; if (newH < 1) newH = 1;

    Microsoft::WRL::ComPtr<IWICBitmapScaler> scaler;
    hr = m_wicFactory->CreateBitmapScaler(scaler.GetAddressOf());
    if (SUCCEEDED(hr)) {
        scaler->Initialize(converter.Get(), newW, newH, WICBitmapInterpolationModeHighQualityCubic);
    }

    std::vector<BYTE> buffer(newW * newH * 4);
    hr = scaler ? scaler->CopyPixels(nullptr, newW * 4, static_cast<UINT>(buffer.size()), buffer.data())
                : converter->CopyPixels(nullptr, newW * 4, static_cast<UINT>(buffer.size()), buffer.data());
    if (FAILED(hr)) return nullptr;

    D2D1_BITMAP_PROPERTIES props = {};
    props.pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED);
    props.dpiX = 96.0f; props.dpiY = 96.0f;

    Microsoft::WRL::ComPtr<ID2D1Bitmap> bitmap;
    hr = m_renderTarget->CreateBitmap(D2D1::SizeU(newW, newH), buffer.data(), newW * 4, &props, bitmap.GetAddressOf());
    if (FAILED(hr)) return nullptr;

    return bitmap;
}

Microsoft::WRL::ComPtr<ID2D1Bitmap> IconLoader::CreateBitmapFromHICON(HICON hIcon, int size) {
    if (!m_renderTarget) return nullptr;

    ICONINFO ii = { sizeof(ii) };
    if (!GetIconInfo(hIcon, &ii)) return nullptr;

    BITMAP bm;
    GetObject(ii.hbmColor ? ii.hbmColor : ii.hbmMask, sizeof(bm), &bm);

    int iconSize = size;
    HDC hDC = GetDC(nullptr);
    HDC hMemDC = CreateCompatibleDC(hDC);

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = iconSize;
    bmi.bmiHeader.biHeight = -iconSize;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HBITMAP hDIB = CreateDIBSection(hMemDC, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    HGDIOBJ hOld = SelectObject(hMemDC, hDIB);

    DrawIconEx(hMemDC, 0, 0, hIcon, iconSize, iconSize, 0, nullptr, DI_NORMAL);

    std::vector<BYTE> buffer(iconSize * iconSize * 4);
    memcpy(buffer.data(), bits, buffer.size());

    // Premultiply alpha
    for (int i = 0; i < iconSize * iconSize; ++i) {
        BYTE a = buffer[i*4+3];
        if (a < 255) {
            buffer[i*4]   = (buffer[i*4]   * a) / 255;
            buffer[i*4+1] = (buffer[i*4+1] * a) / 255;
            buffer[i*4+2] = (buffer[i*4+2] * a) / 255;
        }
    }

    SelectObject(hMemDC, hOld);
    DeleteObject(hDIB);
    DeleteDC(hMemDC);
    ReleaseDC(nullptr, hDC);

    if (ii.hbmColor) DeleteObject(ii.hbmColor);
    if (ii.hbmMask) DeleteObject(ii.hbmMask);

    D2D1_BITMAP_PROPERTIES props = {};
    props.pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED);
    props.dpiX = 96.0f; props.dpiY = 96.0f;

    Microsoft::WRL::ComPtr<ID2D1Bitmap> bitmap;
    HRESULT hr = m_renderTarget->CreateBitmap(D2D1::SizeU(iconSize, iconSize), buffer.data(), iconSize * 4, &props, bitmap.GetAddressOf());
    if (FAILED(hr)) return nullptr;
    return bitmap;
}

LoadedIcon IconLoader::LoadFromExe(const std::wstring& exePath, int size) {
    LoadedIcon icon;
    icon.path = exePath;
    icon.isAnimated = false;
    icon.frameCount = 1;

    // Try IShellItemImageFactory first (best quality, supports UWP)
    Microsoft::WRL::ComPtr<IShellItemImageFactory> imageFactory;
    HRESULT hr = SHCreateItemFromParsingName(exePath.c_str(), nullptr, IID_PPV_ARGS(&imageFactory));
    if (SUCCEEDED(hr) && imageFactory) {
        SIZE sz = { size, size };
        HBITMAP hBmp = nullptr;
        hr = imageFactory->GetImage(sz, SIIGBF_RESIZETOFIT, &hBmp);
        if (SUCCEEDED(hr) && hBmp) {
            // Convert HBITMAP to ID2D1Bitmap via WIC
            Microsoft::WRL::ComPtr<IWICBitmap> wicBmp;
            hr = m_wicFactory->CreateBitmapFromHBITMAP(hBmp, nullptr, WICBitmapIgnoreAlpha, &wicBmp);
            DeleteObject(hBmp);
            if (SUCCEEDED(hr) && wicBmp) {
                Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
                m_wicFactory->CreateFormatConverter(converter.GetAddressOf());
                converter->Initialize(wicBmp.Get(), GUID_WICPixelFormat32bppPBGRA,
                    WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeMedianCut);
                UINT w, h;
                converter->GetSize(&w, &h);
                std::vector<BYTE> buffer(w * h * 4);
                converter->CopyPixels(nullptr, w * 4, static_cast<UINT>(buffer.size()), buffer.data());
                D2D1_BITMAP_PROPERTIES props = {};
                props.pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED);
                props.dpiX = 96; props.dpiY = 96;
                Microsoft::WRL::ComPtr<ID2D1Bitmap> bmp;
                m_renderTarget->CreateBitmap(D2D1::SizeU(w, h), buffer.data(), w*4, &props, bmp.GetAddressOf());
                if (bmp) {
                    icon.frames.push_back(bmp);
                    icon.width = w; icon.height = h;
                    return icon;
                }
            }
        }
    }

    // Fallback: ExtractIconEx
    HICON hIconLarge = nullptr, hIconSmall = nullptr;
    int extracted = ExtractIconExW(exePath.c_str(), 0, &hIconLarge, &hIconSmall, 1);
    if (extracted > 0 && hIconLarge) {
        auto bmp = CreateBitmapFromHICON(hIconLarge, size);
        if (bmp) {
            icon.frames.push_back(bmp);
            icon.width = size; icon.height = size;
        }
        DestroyIcon(hIconLarge);
    }
    if (hIconSmall) DestroyIcon(hIconSmall);

    // Last fallback: SHGetFileInfo
    if (icon.frames.empty()) {
        SHFILEINFOW sfi = {};
        if (SHGetFileInfoW(exePath.c_str(), 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_LARGEICON)) {
            auto bmp = CreateBitmapFromHICON(sfi.hIcon, size);
            if (bmp) {
                icon.frames.push_back(bmp);
                icon.width = size; icon.height = size;
            }
            DestroyIcon(sfi.hIcon);
        }
    }

    return icon;
}

LoadedIcon IconLoader::LoadFromFile(const std::wstring& filePath, int size) {
    LoadedIcon icon;
    icon.path = filePath;
    icon.isAnimated = false;
    icon.frameCount = 1;

    auto bmp = DecodeToBitmap(filePath, size);
    if (bmp) {
        icon.frames.push_back(bmp);
        D2D1_SIZE_F sz = bmp->GetSize();
        icon.width = static_cast<int>(sz.width);
        icon.height = static_cast<int>(sz.height);
    }
    return icon;
}

bool IconLoader::DecodeAnimated(const std::wstring& path, int targetSize, LoadedIcon& outIcon) {
    if (!m_wicFactory) return false;

    Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
    HRESULT hr = m_wicFactory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ,
        WICDecodeMetadataCacheOnDemand, decoder.GetAddressOf());
    if (FAILED(hr)) return false;

    UINT frameCount = 0;
    decoder->GetFrameCount(&frameCount);
    if (frameCount <= 1) return false;

    outIcon.isAnimated = true;
    outIcon.frameCount = static_cast<int>(frameCount);

    // Read frame delay from metadata
    Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> firstFrame;
    decoder->GetFrame(0, firstFrame.GetAddressOf());
    Microsoft::WRL::ComPtr<IWICMetadataQueryReader> metaReader;
    hr = firstFrame->GetMetadataQueryReader(metaReader.GetAddressOf());
    if (SUCCEEDED(hr) && metaReader) {
        PROPVARIANT prop;
        PropVariantInit(&prop);
        hr = metaReader->GetMetadataByName(L"/grctlext/Delay", &prop);
        if (SUCCEEDED(hr) && prop.vt == VT_UI2) {
            outIcon.frameDelayMs = prop.uiVal * 10; // GIF delay is in 1/100s
        }
        PropVariantClear(&prop);
    }
    if (outIcon.frameDelayMs < 20) outIcon.frameDelayMs = 100;

    for (UINT i = 0; i < frameCount; ++i) {
        Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
        hr = decoder->GetFrame(i, frame.GetAddressOf());
        if (FAILED(hr)) continue;

        Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
        m_wicFactory->CreateFormatConverter(converter.GetAddressOf());
        converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppPBGRA,
            WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeMedianCut);

        UINT w, h;
        converter->GetSize(&w, &h);
        float scale = static_cast<float>(targetSize) / std::max(w, h);
        UINT newW = static_cast<UINT>(w * scale);
        UINT newH = static_cast<UINT>(h * scale);
        if (newW < 1) newW = 1; if (newH < 1) newH = 1;

        Microsoft::WRL::ComPtr<IWICBitmapScaler> scaler;
        m_wicFactory->CreateBitmapScaler(scaler.GetAddressOf());
        scaler->Initialize(converter.Get(), newW, newH, WICBitmapInterpolationModeHighQualityCubic);

        std::vector<BYTE> buffer(newW * newH * 4);
        scaler->CopyPixels(nullptr, newW * 4, static_cast<UINT>(buffer.size()), buffer.data());

        D2D1_BITMAP_PROPERTIES props = {};
        props.pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED);
        props.dpiX = 96; props.dpiY = 96;

        Microsoft::WRL::ComPtr<ID2D1Bitmap> bitmap;
        m_renderTarget->CreateBitmap(D2D1::SizeU(newW, newH), buffer.data(), newW * 4, &props, bitmap.GetAddressOf());
        if (bitmap) {
            outIcon.frames.push_back(bitmap);
            outIcon.width = newW;
            outIcon.height = newH;
        }
    }

    return !outIcon.frames.empty();
}

LoadedIcon IconLoader::LoadAnimated(const std::wstring& filePath, int size) {
    LoadedIcon icon;
    icon.path = filePath;
    if (DecodeAnimated(filePath, size, icon)) {
        return icon;
    }
    // Fallback to static
    return LoadFromFile(filePath, size);
}

LoadedIcon IconLoader::LoadSystemIcon(const std::wstring& shellPath, int size) {
    LoadedIcon icon;
    icon.path = shellPath;
    icon.isAnimated = false;
    icon.frameCount = 1;

    SHFILEINFOW sfi = {};
    if (SHGetFileInfoW(shellPath.c_str(), 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_LARGEICON)) {
        auto bmp = CreateBitmapFromHICON(sfi.hIcon, size);
        if (bmp) {
            icon.frames.push_back(bmp);
            icon.width = size; icon.height = size;
        }
        DestroyIcon(sfi.hIcon);
    }
    return icon;
}

void IconLoader::ReleaseIcon(LoadedIcon& icon) {
    icon.frames.clear();
    icon.frameCount = 0;
    icon.isAnimated = false;
}

LoadedIcon IconLoader::LoadFromHICON(HICON hIcon, int size) {
    LoadedIcon result;
    if (!m_renderTarget || !hIcon) return result;

    Microsoft::WRL::ComPtr<IWICImagingFactory> wicFactory;
    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wicFactory));
    if (FAILED(hr)) return result;

    Microsoft::WRL::ComPtr<IWICBitmap> wicBitmap;
    hr = wicFactory->CreateBitmapFromHICON(hIcon, &wicBitmap);
    if (FAILED(hr)) return result;

    Microsoft::WRL::ComPtr<ID2D1Bitmap> d2dBitmap;
    hr = m_renderTarget->CreateBitmapFromWicBitmap(wicBitmap.Get(), &d2dBitmap);
    if (SUCCEEDED(hr) && d2dBitmap) {
        result.frames.push_back(d2dBitmap);
        result.width = size;
        result.height = size;
    }
    return result;
}