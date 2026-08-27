#include "WallpaperEngineIntegration.h"
#include "../Utils/Logger.h"
#include <tlhelp32.h>
#include <shlobj.h>

bool WallpaperEngineIntegration::IsWallpaperEngineRunning() {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return false;
    PROCESSENTRY32W pe = { sizeof(pe) };
    bool found = false;
    if (Process32FirstW(snap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, L"wallpaper32.exe") == 0 || 
                _wcsicmp(pe.szExeFile, L"wallpaper64.exe") == 0) {
                found = true; break;
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return found;
}

std::wstring WallpaperEngineIntegration::GetWallpaperEnginePath() {
    // Check Steam default path
    wchar_t programFiles[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_PROGRAM_FILESX86, nullptr, 0, programFiles))) {
        std::wstring path = std::wstring(programFiles) + L"\\Steam\\steamapps\\common\\wallpaper_engine\\wallpaper64.exe";
        if (GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES) return path;
    }
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_PROGRAM_FILES, nullptr, 0, programFiles))) {
        std::wstring path = std::wstring(programFiles) + L"\\Steam\\steamapps\\common\\wallpaper_engine\\wallpaper64.exe";
        if (GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES) return path;
    }
    return L"";
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

bool WallpaperEngineIntegration::IsUsingLiveWallpaper() {
    return IsWallpaperEngineRunning() && IsWallpaperVisible();
}

void WallpaperEngineIntegration::PauseWallpaper() {
    // Send pause command to Wallpaper Engine via window message or named pipe
    // This is a placeholder for WE API integration
    HWND weWnd = FindWindowW(L"WallpaperEngineNativeWindow", nullptr);
    if (weWnd) {
        SendMessageW(weWnd, WM_APP + 0x100, 0, 0); // Custom pause message
    }
}

void WallpaperEngineIntegration::ResumeWallpaper() {
    HWND weWnd = FindWindowW(L"WallpaperEngineNativeWindow", nullptr);
    if (weWnd) {
        SendMessageW(weWnd, WM_APP + 0x101, 0, 0); // Custom resume message
    }
}

HWND WallpaperEngineIntegration::FindWallpaperWindow() {
    HWND hProgman = FindWindowW(L"Progman", L"Program Manager");
    if (!hProgman) return nullptr;
    HWND hWorkerW = nullptr;
    do {
        hWorkerW = FindWindowExW(nullptr, hWorkerW, L"WorkerW", nullptr);
        if (hWorkerW && FindWindowExW(hWorkerW, nullptr, L"SHELLDLL_DefView", nullptr)) {
            return FindWindowExW(nullptr, hWorkerW, L"WorkerW", nullptr);
        }
    } while (hWorkerW);
    return nullptr;
}