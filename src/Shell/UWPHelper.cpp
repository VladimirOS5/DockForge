#include "UWPHelper.h"
#include <windows.h>
#include <shobjidl_core.h>
#include <propsys.h>
#include <appmodel.h>

bool UWPHelper::IsUWPWindow(HWND hwnd) {
    std::wstring aumid = GetAppUserModelId(hwnd);
    return !aumid.empty() && aumid.find(L"!") != std::wstring::npos;
}

std::wstring UWPHelper::GetUWPAppId(HWND hwnd) {
    return GetAppUserModelId(hwnd);
}

std::wstring UWPHelper::GetAppUserModelId(HWND hwnd) {
    IPropertyStore* ps = nullptr;
    std::wstring result;
    if (SUCCEEDED(SHGetPropertyStoreForWindow(hwnd, IID_PPV_ARGS(&ps)))) {
        PROPVARIANT pv; PropVariantInit(&pv);
        if (SUCCEEDED(ps->GetValue(PKEY_AppUserModel_ID, &pv))) {
            if (pv.vt == VT_LPWSTR) result = pv.pwszVal;
            PropVariantClear(&pv);
        }
        ps->Release();
    }
    return result;
}

std::wstring UWPHelper::GetPackageFamilyName(const std::wstring& aumid) {
    size_t pos = aumid.find(L'!');
    return (pos != std::wstring::npos) ? aumid.substr(0, pos) : L"";
}

std::wstring UWPHelper::GetDisplayNameFromAUMID(const std::wstring& aumid) {
    (void)aumid; return L"";
}

std::wstring UWPHelper::GetPackagePath(const std::wstring& packageFamilyName) {
    (void)packageFamilyName; return L"";
}