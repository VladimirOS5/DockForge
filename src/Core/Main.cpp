#include <windows.h>
#include "Application.h"
#include "../Utils/Logger.h"
#include "../Utils/FallbackManager.h"
#include "../Updater/VersionInfo.h"

#define INSTANCE_MUTEX L"DockForge_SingleInstance_Mutex"

// Command-line handlers for updater integration
enum class LaunchMode {
    Normal,
    AfterUpdate,      // Launched by installer after update
    Rollback,         // User requested rollback
    SafeMode,         // Force safe mode
    SelfTest,         // Run tests and exit
    Uninstall         // Cleanup and exit
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

    // Self-test mode: run diagnostics and exit
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

    // Uninstall mode: cleanup
    if (mode == LaunchMode::Uninstall) {
        // Remove crash flags, old installers, temp files
        wchar_t localAppData[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, localAppData))) {
            std::filesystem::path dockForgeDir = std::filesystem::path(localAppData) / L"DockForge";
            try {
                std::filesystem::remove_all(dockForgeDir);
            } catch (...) {}
        }
        return 0;
    }

    // Single instance check
    HANDLE hMutex = CreateMutexW(nullptr, TRUE, INSTANCE_MUTEX);
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        if (mode == LaunchMode::AfterUpdate) {
            // Another instance is running (old version), wait for it to close
            // The installer should have already closed it, but just in case
            WaitForSingleObject(hMutex, 10000);
        } else {
            MessageBoxW(nullptr, L"DockForge is already running.", L"DockForge", MB_OK | MB_ICONINFORMATION);
            if (hMutex) CloseHandle(hMutex);
            return 0;
        }
    }

    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    // Safe mode force
    if (mode == LaunchMode::SafeMode) {
        Config::Instance().LoadDefaults();
        Config::Instance().GetMutable().safeMode = true;
        Config::Instance().SaveToFile();
    }

    // Rollback handling
    if (mode == LaunchMode::Rollback) {
        FallbackManager::Instance().RollbackUpdate();
        ShowRollbackSuccess();
        if (hMutex) { ReleaseMutex(hMutex); CloseHandle(hMutex); }
        return 0;
    }

    Application app;
    if (!app.Initialize(hInstance)) {
        if (hMutex) { ReleaseMutex(hMutex); CloseHandle(hMutex); }
        return 1;
    }

    int result = app.Run();
    app.Shutdown();

    // After-update notification
    if (mode == LaunchMode::AfterUpdate) {
        ShowUpdateSuccess();
    }

    // Restart handling
    if (app.IsRestartRequested()) {
        wchar_t exePath[MAX_PATH];
        GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        ShellExecuteW(nullptr, L"open", exePath, nullptr, nullptr, SW_SHOW);
    }

    if (hMutex) { ReleaseMutex(hMutex); CloseHandle(hMutex); }
    return result;
}
