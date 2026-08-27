#pragma once
#include <windows.h>
#include <string>

class WallpaperEngineIntegration {
public:
    static bool IsWallpaperEngineRunning();
    static bool IsWallpaperVisible();
    static std::wstring GetWallpaperPath();
    static std::wstring GetWallpaperEnginePath();
    static bool IsUsingLiveWallpaper();
    static void PauseWallpaper();
    static void ResumeWallpaper();
private:
    static HWND FindWallpaperWindow();
};