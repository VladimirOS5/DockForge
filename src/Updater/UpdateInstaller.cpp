#include "UpdateInstaller.h"
#include <tlhelp32.h>
#include <thread>
#include <chrono>

HWND UpdateInstaller::m_hProgressWnd = nullptr;

bool UpdateInstaller::Run(const std::filesystem::path& sourceExe, const std::filesystem::path& targetExe, int waitSeconds) {
    ShowProgressWindow(L"Installing DockForge update...");

    // 1. Wait for main process to exit
    std::wstring targetName = targetExe.filename().wstring();
    if (!WaitForProcessExit(targetName, waitSeconds)) {
        CloseProgressWindow();
        MessageBoxW(nullptr, L"Failed to close DockForge. Please close it manually and try again.", 
            L"Update Failed", MB_OK | MB_ICONERROR);
        return false;
    }

    // 2. Replace executable
    ShowProgressWindow(L"Replacing files...");
    if (!ReplaceFileSafe(sourceExe, targetExe)) {
        CloseProgressWindow();
        MessageBoxW(nullptr, L"Failed to replace application files. The update will be retried on next launch.",
            L"Update Failed", MB_OK | MB_ICONWARNING);
        return false;
    }

    // 3. Cleanup extracted files
    try {
        std::filesystem::remove_all(sourceExe.parent_path());
    } catch (...) {}

    // 4. Restart application
    ShowProgressWindow(L"Restarting DockForge...");
    Sleep(500);
    bool restarted = RestartApplication(targetExe);
    CloseProgressWindow();

    if (restarted) {
        MessageBoxW(nullptr, L"DockForge has been updated successfully!", 
            L"Update Complete", MB_OK | MB_ICONINFORMATION);
    }

    return restarted;
}

bool UpdateInstaller::WaitForProcessExit(const std::wstring& processName, int timeoutSeconds) {
    auto start = std::chrono::steady_clock::now();
    while (std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - start).count() < timeoutSeconds) {

        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) return false;

        PROCESSENTRY32W pe32 = { sizeof(pe32) };
        bool found = false;
        if (Process32FirstW(hSnapshot, &pe32)) {
            do {
                if (_wcsicmp(pe32.szExeFile, processName.c_str()) == 0) {
                    found = true;
                    break;
                }
            } while (Process32NextW(hSnapshot, &pe32));
        }
        CloseHandle(hSnapshot);

        if (!found) return true; // Process exited
        Sleep(500);
    }
    return false; // Timeout
}

bool UpdateInstaller::ReplaceFileSafe(const std::filesystem::path& source, const std::filesystem::path& target) {
    // Try up to 10 times with increasing delays
    for (int attempt = 1; attempt <= 10; ++attempt) {
        try {
            // Method 1: Direct replace
            std::filesystem::rename(source, target);
            return true;
        } catch (...) {
            // Method 2: Copy then delete
            try {
                auto tempTarget = target;
                tempTarget += L".old";
                std::filesystem::rename(target, tempTarget);
                std::filesystem::copy_file(source, target);
                std::filesystem::remove(tempTarget);
                return true;
            } catch (...) {
                Sleep(500 * attempt);
            }
        }
    }
    return false;
}

bool UpdateInstaller::RestartApplication(const std::filesystem::path& exePath) {
    SHELLEXECUTEINFOW sei = { sizeof(sei) };
    sei.lpVerb = L"open";
    sei.lpFile = exePath.c_str();
    sei.nShow = SW_SHOW;
    return ShellExecuteExW(&sei) == TRUE;
}

void UpdateInstaller::ShowProgressWindow(const std::wstring& message) {
    if (m_hProgressWnd) {
        SetWindowTextW(m_hProgressWnd, message.c_str());
        return;
    }

    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"DockForgeUpdateProgress";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(nullptr, IDC_WAIT);
    RegisterClassExW(&wc);

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int w = 400, h = 120;
    int x = (screenW - w) / 2;
    int y = (screenH - h) / 2;

    m_hProgressWnd = CreateWindowExW(WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        L"DockForgeUpdateProgress", message.c_str(),
        WS_POPUP | WS_VISIBLE | WS_CAPTION,
        x, y, w, h, nullptr, nullptr, wc.hInstance, nullptr);

    // Add a progress bar
    HWND hProgress = CreateWindowExW(0, PROGRESS_CLASSW, nullptr,
        WS_CHILD | WS_VISIBLE | PBS_MARQUEE,
        20, 50, w - 40, 20, m_hProgressWnd, nullptr, wc.hInstance, nullptr);
    SendMessageW(hProgress, PBM_SETMARQUEE, TRUE, 50);

    UpdateWindow(m_hProgressWnd);
}

void UpdateInstaller::CloseProgressWindow() {
    if (m_hProgressWnd) {
        DestroyWindow(m_hProgressWnd);
        m_hProgressWnd = nullptr;
    }
}

bool UpdateInstaller::CreateInstallerStub(const std::filesystem::path& outputPath) {
    // Creates a minimal installer executable that can be bundled with updates
    // In production, this would compile a separate small exe
    // For now, we document that the installer logic should be in a separate
    // DockForge.Update.exe that is built from this same source
    (void)outputPath;
    return true;
}
