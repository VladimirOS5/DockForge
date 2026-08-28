#include "StartButton.h"
#include "../Utils/Logger.h"
#include <shellapi.h>  // FIX: added for ShellExecuteW

std::wstring StartButton::GetSkinIconPath(const std::string& skin) {
    if (skin == "default") return L"shell:::{48e7caab-b918-4a58-a94d-5053194fdb18}";
    if (skin == "win10") return L"C:/Windows/Branding/ShellBrd/mpr.dll,101";
    if (skin == "win11") return L"C:/Windows/Branding/ShellBrd/mpr.dll,102";
    return L"shell:::{48e7caab-b918-4a58-a94d-5053194fdb18}";
}

void StartButton::OpenStartMenu() {
    LOG_INFO("Opening Start Menu");
    ShellExecuteW(nullptr, L"open", L"explorer.exe", L"shell:::{48e7caab-b918-4a58-a94d-5053194fdb18}", nullptr, SW_SHOW);
}
