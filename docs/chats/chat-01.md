# Chat 01 — Архитектура и скелет проекта

## Цель
Создать рабочий скелет DockForge: пустое окно Dock, скрытие стандартной панели задач Windows, watchdog-процесс для fallback, логгер.

## Что реализовано

### Core
- **Application** — главный цикл инициализации/завершения. Single-instance через именованный mutex.
- **DockWindow** — layered topmost окно (`WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TOPMOST`).
  - Центрировано внизу экрана с отступами `DOCK_MARGIN`
  - Скруглённые углы (`DOCK_CORNER_RADIUS = 16`)
  - Color-key прозрачность: чёрный цвет = прозрачный
  - DPI-aware (`DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2`)
- **D2DRenderer** — инициализация Direct2D, базовая отрисовка:
  - `Clear`, `FillRect`, `DrawRoundedRect`
  - Обработка потери устройства (`D2DERR_RECREATE_TARGET`)

### Shell
- **TaskbarHider** — скрытие панели задач через `SHAppBarMessage`:
  1. `ABS_AUTOHIDE` + сдвиг за пределы экрана (`ABM_SETPOS`)
  2. `ShowWindow(SW_HIDE)` для `Shell_TrayWnd`, `Start`, `Shell_SecondaryTrayWnd`
  3. Полное восстановление при выходе (позиция + состояние)

### Utils
- **Logger** — потокобезопасный singleton:
  - 5 файлов ротации по 10 МБ
  - Уровни: Debug, Info, Warning, Error, Fatal
  - Путь: `%LocalAppData%/DockForge/logs/DockForge.log`
  - Макросы `LOG_DEBUG`, `LOG_INFO` и т.д. с `__FILE__` и `__LINE__`

### Watchdog
- **DockForge.Watchdog.exe** — отдельный процесс:
  - Мониторинг `DockForge.exe` через `CreateToolhelp32Snapshot`
  - При краше: восстановление панели задач + перезапуск DockForge (до 3 попыток)
  - Сигнализация через именованный mutex `DockForge_Watchdog_Mutex`

## Сборка

```bash
mkdir build && cd build
cmake .. -A x64
cmake --build . --config Release
```

## Запуск
1. Запустить `watchdog\Release\DockForge.Watchdog.exe` (опционально, но рекомендуется)
2. Запустить `src\Release\DockForge.exe`
3. Должна появиться тёмная панель внизу экрана со скруглёнными углами, стандартная панель скрыта
4. При закрытии DockForge (Alt+F4 или крестик) — стандартная панель восстанавливается

## Архитектурные решения
- **Direct2D вместо D3D** на этом этапе: проще, меньше кода, достаточно для 2D-рендера. D3D добавим при реализации Liquid Glass шейдеров (Чат 2).
- **Layered window + color key** вместо `UpdateLayeredWindowIndirect`: проще для начала, перейдём на полноценный alpha-blended layered window при добавлении blur.
- **Watchdog как отдельный exe**, а не поток: если основной процесс падает с access violation, поток внутри него тоже умрёт. Отдельный процесс гарантирует fallback.

## Следующий чат
**Чат 02 — Рендер-движок и эффекты**: Gaussian Blur, Acrylic, Liquid Glass shader, адаптивный FPS.
