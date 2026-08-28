#include "UWPHelper.h"
#include "../Utils/Logger.h"
#include <shobjidl_core.h>  // FIX: added for IApplicationActivationManager
#include <propkey.h>        // FIX: added for PKEY_AppUserModel_ID
#include <shobjidl.h>
#include <appmodel.h>

bool UWPHelper::IsUWPApp(const std::wstring& path) {
    return path.find(L"WindowsApps") != std::wstring::npos || path.find(L"://") != std::wstring::npos;
}

std::wstring UWPHelper::GetAppUserModelId(HWND hwnd) {
    std::wstring aumid;
    Microsoft::WRL::ComPtr<IPropertyStore> propStore;
    if (SUCCEEDED(SHGetPropertyStoreForWindow(hwnd, IID_PPV_ARGS(&propStore)))) {
        PROPVARIANT pv;
        PropVariantInit(&pv);
        if (SUCCEEDED(propStore->GetValue(PKEY_AppUserModel_ID, &pv))) {
            if (pv.vt == VT_LPWSTR && pv.pwszVal) {
                aumid = pv.pwszVal;
            }
            PropVariantClear(&pv);
        }
    }
    return aumid;
}

bool UWPHelper::LaunchUWPApp(const std::wstring& aumid) {
    if (aumid.empty()) return false;
    Microsoft::WRL::ComPtr<IApplicationActivationManager> activator;
    HRESULT hr = CoCreateInstance(CLSID_ApplicationActivationManager, nullptr,
        CLSCTX_LOCAL_SERVER, IID_PPV_ARGS(&activator));
    if (FAILED(hr)) { LOG_ERROR("Failed to create ApplicationActivationManager"); return false; }
    DWORD pid = 0;
    hr = activator->ActivateApplication(aumid.c_str(), nullptr, AO_NONE, &pid);
    if (SUCCEEDED(hr)) {
        LOG_INFO("Launched UWP app: " + std::string(aumid.begin(), aumid.end()));
        return true;
    }
    LOG_ERROR("Failed to launch UWP app: " + std::to_string(hr));
    return false;
}
