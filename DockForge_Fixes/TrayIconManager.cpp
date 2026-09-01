#include "TrayIconManager.h"
#include "../Utils/Logger.h"
#include <commctrl.h>

TrayIconManager& TrayIconManager::Instance() {
    static TrayIconManager instance;
    return instance;
}

void TrayIconManager::Initialize(HINSTANCE hInstance) {
    (void)hInstance;  // Optional parameter for compatibility
    Refresh();
    m_timerId = SetTimer(nullptr, 0, 2000, TimerProc);
    LOG_INFO("TrayIconManager initialized with " + std::to_string(m_icons.size()) + " icons");
}

void TrayIconManager::Shutdown() {
    if (m_timerId) {
        KillTimer(nullptr, m_timerId);
        m_timerId = 0;
    }
    LOG_INFO("TrayIconManager shutdown");
}

void TrayIconManager::SetCallback(std::function<void()> onChange) {
    m_callback = onChange;
}

HWND TrayIconManager::FindTrayToolbar() {
    HWND hTray = FindWindowW(L"Shell_TrayWnd", nullptr);
    if (!hTray) return nullptr;
    HWND hNotify = FindWindowExW(hTray, nullptr, L"TrayNotifyWnd", nullptr);
    if (!hNotify) return nullptr;
    HWND hPager = FindWindowExW(hNotify, nullptr, L"SysPager", nullptr);
    if (!hPager) return nullptr;
    HWND hToolbar = FindWindowExW(hPager, nullptr, L"ToolbarWindow32", nullptr);
    return hToolbar;
}

bool TrayIconManager::ReadTrayButtons(HWND hToolbar, std::vector<TrayIconInfo>& icons) {
    DWORD pid = 0;
    GetWindowThreadProcessId(hToolbar, &pid);
    HANDLE hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_VM_OPERATION | PROCESS_VM_WRITE, FALSE, pid);
    if (!hProcess) return false;

    int count = static_cast<int>(SendMessageW(hToolbar, TB_BUTTONCOUNT, 0, 0));
    if (count <= 0) { CloseHandle(hProcess); return true; }

    SIZE_T tbButtonSize = sizeof(TBBUTTON);
    LPVOID pRemoteBuffer = VirtualAllocEx(hProcess, nullptr, tbButtonSize, MEM_COMMIT, PAGE_READWRITE);
    if (!pRemoteBuffer) { CloseHandle(hProcess); return false; }

    icons.clear();
    icons.reserve(count);

    for (int i = 0; i < count; ++i) {
        if (!SendMessageW(hToolbar, TB_GETBUTTON, i, reinterpret_cast<LPARAM>(pRemoteBuffer))) continue;

        TBBUTTON btn = {};
        SIZE_T read = 0;
        if (!ReadProcessMemory(hProcess, pRemoteBuffer, &btn, sizeof(TBBUTTON), &read)) continue;
        if (btn.dwData == 0) continue;

        struct TRAYDATA {
            HWND hwnd;
            UINT uID;
            UINT uCallbackMessage;
            DWORD dwReserved[2];
            HICON hIcon;
        };

        TRAYDATA tray = {};
        if (!ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(btn.dwData), &tray, sizeof(TRAYDATA), &read)) continue;

        TrayIconInfo info;
        info.hwnd = tray.hwnd;
        info.id = tray.uID;
        info.callbackMessage = tray.uCallbackMessage;
        info.hIcon = tray.hIcon;

        if (btn.iString != -1) {
            wchar_t* pTooltip = nullptr;
            if (ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(btn.iString), &pTooltip, sizeof(wchar_t*), &read)) {
                if (pTooltip) {
                    wchar_t tooltip[256] = {};
                    ReadProcessMemory(hProcess, pTooltip, tooltip, sizeof(tooltip), &read);
                    info.tooltip = tooltip;
                }
            }
        }

        if (info.hwnd && info.hIcon) icons.push_back(info);
    }

    VirtualFreeEx(hProcess, pRemoteBuffer, 0, MEM_RELEASE);
    CloseHandle(hProcess);
    return true;
}

void TrayIconManager::Refresh() {
    HWND hToolbar = FindTrayToolbar();
    if (!hToolbar) return;

    std::vector<TrayIconInfo> newIcons;
    if (!ReadTrayButtons(hToolbar, newIcons)) return;

    bool changed = (newIcons.size() != m_icons.size());
    if (!changed) {
        for (size_t i = 0; i < newIcons.size(); ++i) {
            if (newIcons[i].hwnd != m_icons[i].hwnd || newIcons[i].id != m_icons[i].id) {
                changed = true; break;
            }
        }
    }
    m_icons = std::move(newIcons);
    if (changed && m_callback) m_callback();
}

std::vector<TrayIconInfo> TrayIconManager::GetIcons() const {
    return m_icons;
}

void TrayIconManager::SendMouseEvent(const TrayIconInfo& icon, DWORD message) {
    if (IsWindow(icon.hwnd)) {
        SendMessageW(icon.hwnd, icon.callbackMessage, icon.id, message);
    }
}

void CALLBACK TrayIconManager::TimerProc(HWND hwnd, UINT msg, UINT_PTR id, DWORD time) {
    Instance().Refresh();
}
