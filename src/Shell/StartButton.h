#pragma once
#include <windows.h>
#include <string>

class StartButton {
public:
    static void OpenStartMenu();
    static void OpenStartMenuSettings();
    static std::wstring GetSkinIconPath(const std::string& skin);
};