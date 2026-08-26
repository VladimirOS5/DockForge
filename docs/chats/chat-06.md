# Chat 06 — Системный трей, меню Пуск, контекстное меню Dock

## Цель

Интеграция системного трея в Dock, кастомная кнопка Пуск, глобальное контекстное меню.

## Что реализовано

### TrayIconManager (src/Shell/TrayIconManager)

- **Чтение тулбара трея Explorer'а** — `FindWindowEx` + `TB_GETBUTTON`
- **ReadProcessMemory** — чтение `TBBUTTON` и `TRAYDATA` из памяти Explorer'а (64-bit)
- **Автообновление** — таймер на 2 секунды, callback при изменении
- **Mouse forwarding** — `WM_LBUTTONUP`, `WM_RBUTTONUP`, `WM_MBUTTONUP` в оригинальное окно
- **Иконки через HICON** — конвертация в D2DBitmap через WIC

### StartButton (src/Shell/StartButton)

- **VK_LWIN эмуляция** — `SendInput` для открытия меню Пуск
- **Скины** — `default`, `classic`, `modern` (пути к системным иконкам)
- **Настройки Пуск** — открытие `ms-settings:personalization-start`

### DockContextMenu (src/Shell/DockContextMenu)

- **ПКМ по пустой области Dock** — Settings, Reload, Task Manager, Exit
- **Task Manager** — `taskmgr.exe`
- **Exit** — `PostMessage(WM_CLOSE)`

### DockWindow (обновлён)

- **IconType enum** — `App`, `Tray`, `StartButton`
- **StartButton** — вставляется в начало Dock (слева), скин через Config
- **Tray icons** — отображаются справа, отделены от приложений
- **Layout** — приложения центрируются, трей прижимается к правому краю
- **ЛКМ**:
  - App: LaunchOrActivate (toggle)
  - Tray: forward `WM_LBUTTONUP`
  - StartButton: `StartButton::OpenStartMenu()`
- **ПКМ**:
  - App: Jump List + управление окнами
  - Tray: forward `WM_RBUTTONUP`
  - Пустая область: `DockContextMenu`
- **СКМ**:
  - App: Close all windows
  - Tray: forward `WM_MBUTTONUP`

### IconLoader (обновлён)

- `LoadFromHICON()` — конвертация HICON (из трея) в D2DBitmap через WIC

### Config (обновлён)

- `showTrayIcons` — вкл/выкл отображение трея
- `showStartButton` — вкл/выкл кнопку Пуск
- `startButtonSkin` — `default`, `classic`, `modern`

## Сборка

```bash
cd build
cmake .. -A x64
cmake --build . --config Release
```
