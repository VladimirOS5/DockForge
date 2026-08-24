#include "ScreenCapture.h"
#include "Logger.h"
#include <wincodec.h>
#include <wrl/client.h>

bool ScreenCapture::CaptureRegion(int x, int y, int w, int h, ID2D1DeviceContext* dc, ID2D1Bitmap** outBitmap) {
    if (!dc || w <= 0 || h <= 0) return false;

    HDC hScreenDC = GetDC(nullptr);
    if (!hScreenDC) { LOG_ERROR("Failed to get screen DC"); return false; }

    HDC hMemDC = CreateCompatibleDC(hScreenDC);
    if (!hMemDC) { ReleaseDC(nullptr, hScreenDC); return false; }

    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, w, h);
    if (!hBitmap) { DeleteDC(hMemDC); ReleaseDC(nullptr, hScreenDC); return false; }

    HGDIOBJ hOld = SelectObject(hMemDC, hBitmap);
    BOOL ok = BitBlt(hMemDC, 0, 0, w, h, hScreenDC, x, y, SRCCOPY | CAPTUREBLT);
    SelectObject(hMemDC, hOld);

    if (!ok) {
        LOG_WARN("BitBlt failed, using fallback color");
        DeleteObject(hBitmap); DeleteDC(hMemDC); ReleaseDC(nullptr, hScreenDC);
        return false;
    }

    Microsoft::WRL::ComPtr<IWICImagingFactory> wicFactory;
    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&wicFactory));
    if (FAILED(hr)) {
        DeleteObject(hBitmap); DeleteDC(hMemDC); ReleaseDC(nullptr, hScreenDC);
        return false;
    }

    Microsoft::WRL::ComPtr<IWICBitmap> wicBitmap;
    hr = wicFactory->CreateBitmapFromHBITMAP(hBitmap, nullptr, WICBitmapIgnoreAlpha, &wicBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hMemDC);
    ReleaseDC(nullptr, hScreenDC);

    if (FAILED(hr)) { LOG_ERROR("WIC CreateBitmapFromHBITMAP failed"); return false; }

    hr = dc->CreateBitmapFromWicBitmap(wicBitmap.Get(), nullptr, outBitmap);
    if (FAILED(hr)) { LOG_ERROR("CreateBitmapFromWicBitmap failed"); return false; }

    return true;
}
