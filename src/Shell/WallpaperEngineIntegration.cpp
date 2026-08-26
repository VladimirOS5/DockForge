#include "WallpaperEngineIntegration.h"
#include "../Utils/Logger.h"
#include <tlhelp32.h>

bool WallpaperEngineIntegration::IsWallpaperEngineRunning() {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return false;
    PROCESSENTRY32W pe = { sizeof(pe) };
    bool found = false;
    if (Process32FirstW(snap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, L"wallpaper32.exe") == 0 || _wcsicmp(pe.szExeFile, L"wallpaper64.exe") == 0) {
                found = true; break;
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return found;
}

bool WallpaperEngineIntegration::IsWallpaperVisible() {
    HWND hProgman = FindWindowW(L"Progman", L"Program Manager");
    if (!hProgman) return true;
    HWND hWorkerW = nullptr;
    do {
        hWorkerW = FindWindowExW(nullptr, hWorkerW, L"WorkerW", nullptr);
        if (hWorkerW) {
            if (FindWindowExW(hWorkerW, nullptr, L"SHELLDLL_DefView", nullptr)) {
                hWorkerW = FindWindowExW(nullptr, hWorkerW, L"WorkerW", nullptr);
                return IsWindowVisible(hWorkerW);
            }
        }
    } while (hWorkerW);
    return true;
}

std::wstring WallpaperEngineIntegration::GetWallpaperPath() {
    wchar_t path[MAX_PATH];
    SystemParametersInfoW(SPI_GETDESKWALLPAPER, MAX_PATH, path, 0);
    return path;
}