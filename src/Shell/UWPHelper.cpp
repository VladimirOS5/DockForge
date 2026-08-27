#include "UWPHelper.h"
#include "../Utils/Logger.h"
#include <tlhelp32.h>
#include <propvarutil.h>
#include <propsys.h>

#pragma comment(lib, "propsys.lib")

bool UWPHelper::IsUWPWindow(HWND hwnd) {
    if (!IsWindow(hwnd)) return false;

    // UWP apps run under ApplicationFrameHost or have immersive class
    wchar_t className[256] = {};
    GetClassNameW(hwnd, className, 256);

    if (wcscmp(className, L"ApplicationFrameWindow") == 0 ||
        wcscmp(className, L"Windows.UI.Core.CoreWindow") == 0) {
        return true;
    }

    // Check process name
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);

    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32W pe = { sizeof(pe) };
    bool isUWP = false;
    if (Process32FirstW(snap, &pe)) {
        do {
            if (pe.th32ProcessID == pid) {
                if (_wcsicmp(pe.szExeFile, L"ApplicationFrameHost.exe") == 0) {
                    isUWP = true;
                }
                break;
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return isUWP;
}

std::wstring UWPHelper::GetUWPAppId(HWND hwnd) {
    // Try to read ApplicationUserModelId from window properties
    // This requires the app to expose it

    // Method 1: Check if window has AppUserModelID property
    IPropertyStore* props = nullptr;
    if (SUCCEEDED(SHGetPropertyStoreForWindow(hwnd, IID_PPV_ARGS(&props)))) {
        PROPVARIANT pv;
        if (SUCCEEDED(props->GetValue(PKEY_AppUserModel_ID, &pv))) {
            std::wstring result(pv.pwszVal);
            PropVariantClear(&pv);
            props->Release();
            return result;
        }
        props->Release();
    }

    return L"";
}

std::vector<UWPAppInfo> UWPHelper::EnumerateInstalledApps() {
    std::vector<UWPAppInfo> apps;
    // Real implementation would use PackageManager API (Windows.Management.Deployment)
    // This requires C++/WinRT or COM interop
    LOG_INFO("UWP enumeration: PackageManager API not implemented in this build");
    return apps;
}

bool UWPHelper::LaunchUWPApp(const std::wstring& appUserModelId) {
    if (appUserModelId.empty()) return false;

    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    IApplicationActivationManager* activator = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_ApplicationActivationManager, nullptr, 
        CLSCTX_LOCAL_SERVER, IID_PPV_ARGS(&activator));
    if (FAILED(hr)) {
        CoUninitialize();
        return false;
    }

    DWORD pid = 0;
    hr = activator->ActivateApplication(appUserModelId.c_str(), nullptr, AO_NONE, &pid);
    activator->Release();
    CoUninitialize();

    return SUCCEEDED(hr);
}

std::wstring UWPHelper::GetUWPWindowTitle(HWND hwnd) {
    if (!IsUWPWindow(hwnd)) return L"";

    wchar_t title[512] = {};
    GetWindowTextW(hwnd, title, 512);
    return std::wstring(title);
}

bool UWPHelper::IsApplicationFrameHost(HWND hwnd) {
    wchar_t className[256] = {};
    GetClassNameW(hwnd, className, 256);
    return wcscmp(className, L"ApplicationFrameWindow") == 0;
}

HWND UWPHelper::FindCoreWindow(HWND frameHost) {
    // UWP apps have a child CoreWindow inside the frame
    return FindWindowExW(frameHost, nullptr, L"Windows.UI.Core.CoreWindow", nullptr);
}
