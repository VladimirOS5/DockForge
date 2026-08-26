#include "StartButton.h"
#include "../Utils/Logger.h"

void StartButton::OpenStartMenu() {
    INPUT inputs[2] = {};
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_LWIN;
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = VK_LWIN;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(2, inputs, sizeof(INPUT));
    LOG_INFO("Start menu opened");
}

void StartButton::OpenStartMenuSettings() {
    ShellExecuteW(nullptr, L"open", L"ms-settings:personalization-start", nullptr, nullptr, SW_SHOW);
}

std::wstring StartButton::GetSkinIconPath(const std::string& skin) {
    if (skin == "classic") return L"C:\\Windows\\System32\\shell32.dll,43";
    if (skin == "modern") return L"C:\\Windows\\System32\\imageres.dll,109";
    return L"C:\\Windows\\System32\\imageres.dll,109";
}