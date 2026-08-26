#pragma once
#include <windows.h>
#include <string>

class WallpaperEngineIntegration {
public:
    static bool IsWallpaperEngineRunning();
    static bool IsWallpaperVisible();
    static std::wstring GetWallpaperPath();
};