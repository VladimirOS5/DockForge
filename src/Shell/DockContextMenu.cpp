#include "DockContextMenu.h"
#include "../Utils/Logger.h"
#include <shellapi.h>
#include "../Settings/SettingsWindow.h"

void DockContextMenu::Show(HWND hwnd, int x, int y) {
    HMENU hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING, 1, L"Settings...");
    AppendMenuW(hMenu, MF_STRING, 2, L"Reload Dock");
    AppendMenuW(hMenu, MF_STRING, 3, L"Task Manager");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, 4, L"Exit DockForge");

    SetForegroundWindow(hwnd);
    int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, x, y, 0, hwnd, nullptr);
    DestroyMenu(hMenu);

    switch (cmd) {
        case 1: SettingsWindow::Instance().Toggle(); break;
        case 2: LOG_INFO("Reload clicked (placeholder)"); break;
        case 3: ShellExecuteW(nullptr, L"open", L"taskmgr.exe", nullptr, nullptr, SW_SHOW); break;
        case 4: PostMessageW(hwnd, WM_CLOSE, 0, 0); break;
    }
}
