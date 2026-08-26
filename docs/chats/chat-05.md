# Chat 05 — Shell интеграция: окна, превью, Jump Lists

## Цель

Dock знает обо всех окнах и умеет с ними работать: переключение, превью, Jump Lists, закрытие.

## Что реализовано

### ShellHookManager (src/Shell/ShellHookManager)

- **Message-only window** — отдельное HWND для `RegisterShellHookWindow`
- **События**: WindowCreated, WindowDestroyed, WindowActivated, WindowRedraw, Flash
- **Callback система** — передача событий в WindowManager

### WindowManager (src/Shell/WindowManager)

- **Кэш окон** — `std::map&lt;HWND, WindowInfo&gt;` с mutex для thread-safety
- **Фильтрация** — исключает tool windows, наши окна, невидимые, слишком маленькие
- **Группировка по процессу** — `GetWindowsForProcess()` для связи иконки Dock с окнами
- **Операции**: Activate, Minimize, Restore, Close, Flash
- **Автообновление** — при событиях от ShellHookManager

### ThumbnailPreview (src/Shell/ThumbnailPreview)

- **DwmRegisterThumbnail** — live preview окна над иконкой Dock
- **DwmUpdateThumbnailProperties** — позиция, opacity, client-area only
- **Автоскрытие** — при уходе мыши с иконки

### JumpListManager (src/Shell/JumpListManager)

- **ICustomDestinationList** — чтение системного Jump List (best-effort)
- **Fallback** — common tasks для известных приложений (Explorer, Notepad)
- **LaunchItem** — запуск через ShellExecuteEx

### DockWindow (обновлён)

- **ЛКМ** — `LaunchOrActivate()`: если окно запущено → переключение/минимизация (toggle), если нет → запуск
- **СКМ (колёсико)** — закрыть все окна приложения
- **ПКМ** — контекстное меню: Jump List + Minimize/Restore/Close + Pin/Unpin
- **Hover** — magnification + thumbnail preview (если окно запущено)
- **Running indicator** — точка под иконкой, обновляется по `WindowManager`
- **UpdateRunningStates()** — периодическая синхронизация состояния

### Config (обновлён)

- `thumbnailPreviews` — вкл/выкл live preview
- `jumpLists` — вкл/выкл Jump Lists
- `windowGrouping` — вкл/выкл группировку

## Сборка

```bash
cd build
cmake .. -A x64
cmake --build . --config Release
```
