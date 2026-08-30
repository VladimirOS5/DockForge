#include "UWPHelper.h"
#include "../Utils/Logger.h"
#include <wrl/client.h>
#include <propsys.h>
#include <shobjidl_core.h>
#include <propkey.h>
#include <shobjidl.h>

bool UWPHelper::IsUWPApp(HWND hwnd) {
    std::wstring aumid = GetAppUserModelId(hwnd);
    return !aumid.empty();
}

std::wstring UWPHelper::GetAppUserModelId(HWND hwnd) {
    Microsoft::WRL::ComPtr<IPropertyStore> propStore;
    HRESULT hr = SHGetPropertyStoreForWindow(hwnd, IID_PPV_ARGS(&propStore));
    if (FAILED(hr)) return L"";
    PROPVARIANT pv;
    PropVariantInit(&pv);
    hr = propStore->GetValue(PKEY_AppUserModel_ID, &pv);
    std::wstring result;
    if (SUCCEEDED(hr) && pv.vt == VT_LPWSTR) {
        result = pv.pwszVal;
    }
    PropVariantClear(&pv);
    return result;
}

std::wstring UWPHelper::GetPackageFamilyName(const std::wstring& aumid) {
    size_t pos = aumid.find(L'!');
    if (pos != std::wstring::npos) {
        return aumid.substr(0, pos);
    }
    return aumid;
}

std::wstring UWPHelper::GetDisplayNameFromAUMID(const std::wstring& aumid) {
    Microsoft::WRL::ComPtr<IApplicationActivationManager> activator;
    HRESULT hr = CoCreateInstance(CLSID_ApplicationActivationManager, nullptr,
        CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&activator));
    if (FAILED(hr)) return L"";
    return L"";
}

std::wstring UWPHelper::GetPackagePath(const std::wstring& packageFamilyName) {
    return L"";
}
