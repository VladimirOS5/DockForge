#pragma once
#include <windows.h>
#include <string>
#include <filesystem>

// Standalone update installer — launched by OTAUpdater to replace the main exe
// This is a separate mini-executable that should be bundled with the update package
class UpdateInstaller {
public:
    static bool Run(const std::filesystem::path& sourceExe, const std::filesystem::path& targetExe, int waitSeconds = 5);
    static bool CreateInstallerStub(const std::filesystem::path& outputPath);
private:
    static bool WaitForProcessExit(const std::wstring& processName, int timeoutSeconds);
    static bool ReplaceFileSafe(const std::filesystem::path& source, const std::filesystem::path& target);
    static bool RestartApplication(const std::filesystem::path& exePath);
    static void ShowProgressWindow(const std::wstring& message);
    static void CloseProgressWindow();
    static HWND m_hProgressWnd;
};
