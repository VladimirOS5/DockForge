#pragma once
#include <string>
#include <windows.h>

class UWPHelper {
public:
    static bool IsUWPWindow(HWND hwnd);
    static std::wstring GetUWPAppId(HWND hwnd);
    static std::wstring GetAppUserModelId(HWND hwnd);
    static std::wstring GetPackageFamilyName(const std::wstring& aumid);
    static std::wstring GetDisplayNameFromAUMID(const std::wstring& aumid);
    static std::wstring GetPackagePath(const std::wstring& packageFamilyName);
};