#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>  // FIX: added for CSIDL_LOCAL_APPDATA, SHGetFolderPathW
#include <filesystem>
#include <iostream>
#include "Application.h"
#include "../Utils/Logger.h"
#include "../Utils/FallbackManager.h"
#include "../Updater/VersionInfo.h"

#define INSTANCE_MUTEX L"DockForge_SingleInstance_Mutex"

enum class LaunchMode {
    Normal, AfterUpdate, Rollback, SafeMode, SelfTest, Uninstall
};

LaunchMode ParseCommandLine(LPWSTR cmdLine) {
    if (!cmdLine) return LaunchMode::Normal;
    std::wstring cl(cmdLine);
    if (cl.find(L"/afterupdate") != std::wstring::npos) return LaunchMode::AfterUpdate;
    if (cl.find(L"/rollback") != std::wstring::npos) return LaunchMode::Rollback;
    if (cl.find(L"/safemode") != std::wstring::npos) return LaunchMode::SafeMode;
    if (cl.find(L"/selftest") != std::wstring::npos) return LaunchMode::SelfTest;
    if (cl.find(L"/uninstall") != std::wstring::npos) return LaunchMode::Uninstall;
    return LaunchMode::Normal;
}

void ShowUpdateSuccess() {
    MessageBoxW(nullptr,
        L"DockForge has been successfully updated!\n\nEnjoy the new features.",
        L"Update Complete", MB_OK | MB_ICONINFORMATION);
}

void ShowRollbackSuccess() {
    MessageBoxW(nullptr,
        L"DockForge has been restored to the previous version.",
        L"Rollback Complete", MB_OK | MB_ICONINFORMATION);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR lpCmdLine, int) {
    LaunchMode mode = ParseCommandLine(lpCmdLine);

    if (mode == LaunchMode::SelfTest) {
        AllocConsole();
        FILE* dummy;
        freopen_s(&dummy, "CONOUT$", "w", stdout);
        std::cout << "DockForge Self-Test Suite\n";
        std::cout << "=========================\n";
        bool ok = FallbackManager::Instance().RunSelfTests();
        std::cout << "\nResult: " << (ok ? "ALL PASSED" : "SOME FAILED") << "\n";
        std::cout << "Press any key to exit...\n";
        system("pause >nul");
        FreeConsole();
        return ok ? 0 : 1;
    }

    if (mode == LaunchMode::Uninstall) {
        wchar_t localAppData[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, localAppData))) {
            std::filesystem::path dockForgeDir = std::filesystem::path(localAppData) / L"DockForge";
            try { std::filesystem::remove_all(dockForgeDir); } catch (...) {}
        }
        return 0;
    }

    HANDLE hMutex = CreateMutexW(nullptr, TRUE, INSTANCE_MUTEX);
    bool ownsMutex = (GetLastError() != ERROR_ALREADY_EXISTS);

    if (!ownsMutex) {
        if (mode == LaunchMode::AfterUpdate) {
            WaitForSingleObject(hMutex, 10000);
        } else {
            MessageBoxW(nullptr, L"DockForge is already running.", L"DockForge", MB_OK | MB_ICONINFORMATION);
            CloseHandle(hMutex);
            return 0;
        }
    }

    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    if (mode == LaunchMode::SafeMode) {
        Config::Instance().LoadDefaults();
        Config::Instance().GetMutable().safeMode = true;
        Config::Instance().SaveToFile();
    }

    if (mode == LaunchMode::Rollback) {
        FallbackManager::Instance().RollbackUpdate();
        ShowRollbackSuccess();
        if (ownsMutex) ReleaseMutex(hMutex);
        CloseHandle(hMutex);
        return 0;
    }

    Application app;
    if (!app.Initialize(hInstance)) {
        if (ownsMutex) ReleaseMutex(hMutex);
        CloseHandle(hMutex);
        return 1;
    }

    int result = app.Run();

    if (mode == LaunchMode::AfterUpdate) {
        ShowUpdateSuccess();
    }

    if (app.IsRestartRequested()) {
        wchar_t exePath[MAX_PATH];
        GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        ShellExecuteW(nullptr, L"open", exePath, nullptr, nullptr, SW_SHOW);
    }

    if (ownsMutex) ReleaseMutex(hMutex);
    CloseHandle(hMutex);
    return result;
}
