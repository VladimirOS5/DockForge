#pragma once
#include <windows.h>
#include <string>
#include <vector>

struct UWPAppInfo {
    std::wstring name;
    std::wstring packageFamilyName;
    std::wstring appUserModelId;
    std::wstring iconPath;
    bool isRunning = false;
};

class UWPHelper {
public:
    static bool IsUWPWindow(HWND hwnd);
    static std::wstring GetUWPAppId(HWND hwnd);
    static std::vector<UWPAppInfo> EnumerateInstalledApps();
    static bool LaunchUWPApp(const std::wstring& appUserModelId);
    static std::wstring GetUWPWindowTitle(HWND hwnd);
    static bool IsApplicationFrameHost(HWND hwnd);
private:
    static HWND FindCoreWindow(HWND frameHost);
};
