#include "JumpListManager.h"
#include "../Utils/Logger.h"
#include <shlobj.h>
#include <propvarutil.h>
#include <propkey.h>
#include <wrl/client.h>

#pragma comment(lib, "propsys")

JumpListManager& JumpListManager::Instance() {
    static JumpListManager instance;
    return instance;
}

std::vector<JumpListItem> JumpListManager::GetJumpList(const std::wstring& exePath) {
    std::vector<JumpListItem> items;
    
    Microsoft::WRL::ComPtr<ICustomDestinationList> destList;
    HRESULT hr = CoCreateInstance(CLSID_DestinationList, nullptr, CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&destList));
    
    if (SUCCEEDED(hr)) {
        UINT minSlots = 0;
        Microsoft::WRL::ComPtr<IObjectArray> removed;
        hr = destList->BeginList(&minSlots, IID_PPV_ARGS(&removed));
        
        if (SUCCEEDED(hr)) {
            Microsoft::WRL::ComPtr<IObjectCollection> objColl;
            hr = destList->GetDestinationList(&objColl);
            
            if (SUCCEEDED(hr) && objColl) {
                UINT count = 0;
                objColl->GetCount(&count);
                for (UINT i = 0; i < count; ++i) {
                    Microsoft::WRL::ComPtr<IShellItem> shellItem;
                    if (SUCCEEDED(objColl->GetAt(i, IID_PPV_ARGS(&shellItem)))) {
                        PWSTR path = nullptr;
                        if (SUCCEEDED(shellItem->GetDisplayName(SIGDN_FILESYSPATH, &path))) {
                            JumpListItem item;
                            item.path = path;
                            item.title = path;
                            size_t pos = item.title.find_last_of(L"\\\\");
                            if (pos != std::wstring::npos) item.title = item.title.substr(pos + 1);
                            items.push_back(item);
                            CoTaskMemFree(path);
                        }
                    }
                }
            }
            destList->AbortList();
        }
    }
    
    if (items.empty()) {
        items = GetRecentFiles(exePath);
    }
    
    auto tasks = GetCommonTasks(exePath);
    items.insert(items.end(), tasks.begin(), tasks.end());
    
    return items;
}

std::vector<JumpListItem> JumpListManager::GetRecentFiles(const std::wstring& exePath) {
    std::vector<JumpListItem> items;
    return items;
}

std::vector<JumpListItem> JumpListManager::GetCommonTasks(const std::wstring& exePath) {
    std::vector<JumpListItem> items;
    
    std::wstring lowerPath = exePath;
    for (auto& c : lowerPath) c = towlower(c);
    
    if (lowerPath.find(L"explorer.exe") != std::wstring::npos) {
        items.push_back({L"Documents", L"", L"shell:Documents", L"", 0});
        items.push_back({L"Downloads", L"", L"shell:Downloads", L"", 0});
    } else if (lowerPath.find(L"notepad.exe") != std::wstring::npos) {
        items.push_back({L"New File", L"notepad.exe", L"", L"", 0});
    }
    
    return items;
}

bool JumpListManager::LaunchItem(const JumpListItem& item) {
    if (item.path.empty() && !item.arguments.empty()) {
        HINSTANCE result = ShellExecuteW(nullptr, L"open", item.arguments.c_str(), nullptr, nullptr, SW_SHOW);
        return reinterpret_cast<int>(result) > 32;
    }
    
    if (!item.path.empty()) {
        SHELLEXECUTEINFOW sei = { sizeof(sei) };
        sei.lpFile = item.path.c_str();
        sei.lpParameters = item.arguments.empty() ? nullptr : item.arguments.c_str();
        sei.nShow = SW_SHOW;
        sei.fMask = SEE_MASK_DEFAULT;
        return ShellExecuteExW(&sei) != FALSE;
    }
    
    return false;
}