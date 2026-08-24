#pragma once
#include <windows.h>
#include <d2d1_1.h>

class ScreenCapture {
public:
    static bool CaptureRegion(int x, int y, int w, int h, ID2D1DeviceContext* dc, ID2D1Bitmap** outBitmap);
};
