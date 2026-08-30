#pragma once
#include <windows.h>
#include <wrl/client.h>
#include <string>
#include <vector>

struct JumpListItem {
    std::wstring title;
    std::wstring path;
    std::wstring arguments;
    std::wstring iconPath;
    int iconIndex = 0;
};

class JumpListManager {
public:
    static JumpListManager& Instance();
    std::vector<JumpListItem> GetJumpList(const std::wstring& exePath);
    void LaunchItem(const JumpListItem& item);

private:
    JumpListManager() = default;
    std::vector<JumpListItem> GetRecentFiles(const std::wstring& exePath);
    std::vector<JumpListItem> GetCommonTasks(const std::wstring& exePath);
};
