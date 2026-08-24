# DockForge — План разработки по чатам

> Файл для отслеживания прогресса. Обновляется после каждого чата.
> Текущая версия: v1.0.0-alpha | Последний чат: 03

---

## Чат 01 — Архитектура и скелет проекта ✅
**Цель:** Работающее пустое окно Dock + скрытие стандартной панели + fallback.

- [x] CMake проект, Direct2D SDK, nlohmann/json (заготовка)
- [x] Layered topmost окно Dock (`WS_EX_LAYERED`, `WS_EX_TOOLWINDOW`, `WS_EX_NOACTIVATE`)
- [x] TaskbarHider: `SHAppBarMessage` скрытие/восстановление панели
- [x] Watchdog процесс (`DockForge.Watchdog.exe`): мониторинг, fallback при краше
- [x] Логгер: singleton, ротация, уровни, `%AppData%/DockForge/logs/`
- [x] Базовая отрисовка: цветной прямоугольник через Direct2D
- [x] Single-instance mutex, DPI awareness

**Deliverable:** Запуск → панель Windows прячется, появляется Dock. При kill → панель восстанавливается.

---

## Чат 02 — Рендер-движок и эффекты ✅
**Цель:** Производительный рендер с blur/acrylic/liquid glass.

- [x] Direct2D device context + WARP offscreen rendering
- [x] Gaussian Blur через `ID2D1Effect`
- [x] Acrylic: blur + noise texture + tint overlay
- [x] Liquid Glass: DisplacementMap + Saturation, анимированная displacement texture
- [x] V-Sync (`DwmFlush`) + adaptive FPS
- [x] Dirty rectangles system (заготовка)
- [x] Screen capture: `BitBlt` + WIC → `ID2D1Bitmap`
- [x] Config singleton: `targetFPS`, `blurRadius`, `tint*`, `refractionStrength`

**Deliverable:** Dock с прозрачным фоном, переключаемые эффекты (F1), blur под Dock.

---

## Чат 03 — Иконки и ресурсы ✅
**Цель:** Загрузка и отрисовка иконок приложений.

- [x] IconLoader: `IShellItemImageFactory`, `ExtractIconEx`, `SHGetFileInfo`, WIC декодер
- [x] HICON → D2D1Bitmap с premultiplied alpha
- [x] TextureAtlas: shelf packing, 1024×1024, UV-координаты, 2px padding
- [x] Анимированные иконки: GIF/APNG через WIC, frame delay из metadata
- [x] BadgeRenderer: красный кружок, цифра, "99+", обводка
- [x] DockIcon struct, demo-иконки (Explorer, Notepad, Calc, Edge, Settings)
- [x] Magnification: плавное увеличение 1.3x при наведении
- [x] Running indicator (точка под иконкой)
- [x] `DrawBitmapFromAtlas()` — отрисовка под-региона по UV

**Deliverable:** Dock показывает реальные иконки с badge'ами, magnification, анимированные иконки.

---

## Чат 04 — Анимационный движок ⏳
**Цель:** Все анимации macOS + кастомные, оптимизированные.

- [ ] TweenEngine: time-based interpolation, easing curves
  - [ ] linear, ease-out, ease-out-back, ease-out-bounce, elastic
- [ ] Magnification: кривая ease-out-circ, настраиваемый `maxScale`, `range`
- [ ] Прыжок иконки при запуске: translate Y + scale + opacity (ease-out-back)
- [ ] Genie Effect: Direct2D mesh deformation (grid 8×8, Bézier curve)
- [ ] Появление/скрытие Dock: slide + fade (настраиваемое направление)
- [ ] Настройки анимаций: скорость (мс), тип easing, отключение
- [ ] Анимация badge (пульсация при новом уведомлении)

**Deliverable:** Плавные анимации при наведении, запуске, сворачивании окна.

---

## Чат 05 — Shell интеграция: окна, превью, Jump Lists ⏳
**Цель:** Dock знает обо всех окнах и умеет с ними работать.

- [ ] ShellHookManager: `RegisterShellHookWindow`, `HSHELL_*` сообщения
- [ ] WindowManager: кэш окон, фильтрация, группировка по `AppUserModelID`
- [ ] Индикаторы запущенных приложений: точка под иконкой (настраиваемый цвет/форма)
- [ ] Thumbnail previews: `DwmRegisterThumbnail` при наведении
- [ ] Jump Lists: `ICustomDestinationList` + `IObjectCollection`, ПКМ
- [ ] Переключение/закрытие/сворачивание окон по клику/СКМ
- [ ] Замена `BitBlt` на `DwmRegisterThumbnail` для screen capture

**Deliverable:** Клик переключает окна, ПКМ показывает Jump List и превью, точки-индикаторы работают.

---

## Чат 06 — Системный трей, меню Пуск, контекстное меню ⏳
**Цель:** Полная замена функций стандартной панели.

- [ ] System Tray: отображение системных иконок в зоне Dock
- [ ] Кастомная кнопка Пуск: замена `Shell_TrayWnd`, отправка `VK_LWIN`
- [ ] Дизайны Пуск: система скинов (SVG/PNG), встроенные наборы
- [ ] Контекстное меню Dock: настройки, выход, список мониторов
- [ ] Интеграция с `ITrayNotify` COM (опционально)

**Deliverable:** Трей-иконки видны в Dock, кнопка Пуск кастомизируется.

---

## Чат 07 — Окно настроек и темизация ⏳
**Цель:** Полный контроль внешнего вида через GUI.

- [ ] Окно настроек: отдельное layered окно, рендер на Direct2D
- [ ] Темы: светлая / тёмная / авто (`WM_SETTINGCHANGE` + реестр)
- [ ] Настройки Dock: размер, положение, auto-hide задержка, magnification
- [ ] Настройки эффектов: тип фона, прозрачность, цвет tint
- [ ] Настройки производительности: FPS limit, качество анимаций
- [ ] Система скинов: JSON-конфиг с цветами, размерами, шрифтами
- [ ] Экспорт/импорт тем
- [ ] Сохранение/загрузка Config в JSON

**Deliverable:** Окно настроек с live-preview, переключение тем.

---

## Чат 08 — Плагиновая система и виджеты ⏳
**Цель:** Расширяемость без перекомпиляции.

- [ ] `IPlugin` C-интерфейс: `Init`, `Update`, `Render`, `GetWidgetInfo`, `OnEvent`
- [ ] PluginManager: загрузка DLL из `%AppData%/DockForge/plugins/`, hot-reload
- [ ] Виджеты на рабочий стол: layered окна, shared Direct2D device
- [ ] Виджеты в Dock: специальные "слоты", рендер плагином
- [ ] Встроенные плагины: DigitalClock, Weather (Open-Meteo), SystemMonitor (PDH), Calendar
- [ ] Sandbox: плагины в отдельном процессе через IPC (опционально)

**Deliverable:** Загружаемые DLL-плагины, виджеты на столе и в Dock.

---

## Чат 09 — Мульти-мониторы и профили производительности ⏳
**Цель:** Работа на нескольких экранах + тонкая оптимизация.

- [ ] MonitorManager: `EnumDisplayMonitors`, Dock-окно для каждого монитора
- [ ] Клонирование настроек: Dock на каждом мониторе идентичен основному
- [ ] Performance Profiles: "Eco", "Balanced", "Performance", "Custom"
- [ ] Оптимизация полноэкрана: `DWMWA_CLOAKED` → пауза анимаций, FPS 1
- [ ] Оптимизация idle: мышь не двигалась 5 сек → снижение FPS
- [ ] Настройки профилей в Config

**Deliverable:** Dock на всех мониторах, переключение профилей, автопауза в играх.

---

## Чат 10 — Анимированные обои и фон Dock ⏳
**Цель:** Эффект Liquid Glass / реактивный фон.

- [ ] Прозрачный viewport: Dock показывает живые обои под ним
- [ ] Audio-reactive шейдер: WASAPI loopback, визуализация волн/частиц
- [ ] Gradient flow: плавно движущийся mesh-градиент
- [ ] Particle system: частицы реагируют на наведение мыши
- [ ] Direct2D custom effect с HLSL (опционально)
- [ ] Интеграция с Wallpaper Engine / Lively Wallpaper

**Deliverable:** Переключаемые фоны под Dock, audio-reactive режим.

---

## Чат 11 — Автообновления (OTA), логирование, инсталлятор ⏳
**Цель:** Готовность к распространению.

- [ ] OTA Updater: проверка версии, скачивание .zip, распаковка, перезапуск
- [ ] Инсталлятор: Inno Setup, установка в `Program Files/DockForge/`, автозагрузка
- [ ] Логирование: ротация, уровни, `%LocalAppData%/DockForge/logs/`
- [ ] Отказоустойчивость: отладка watchdog, тесты fallback
- [ ] Проверка восстановления панели при `taskkill /F`

**Deliverable:** Installer .exe, работающие автообновления, стабильный fallback.

---

## Чат 12 — Полировка, тестирование, релиз ⏳
**Цель:** Production-ready продукт.

- [ ] Memory leak detection: `_CrtDumpMemoryLeaks`, VS Diagnostic Tools
- [ ] Стабильность: тест 72 часа непрерывной работы
- [ ] Edge cases: UWP, elevated окна, DPI scaling (100%, 125%, 150%, 200%), HDR
- [ ] Финальная оптимизация: draw calls, текстурный атлас
- [ ] Документация: `docs/api/plugin_api.md`, `docs/user_guide.md`
- [ ] README с гифками, скриншотами

**Deliverable:** v1.0.0, готовый к публикации релиз.

---

## Архитектурные решения (ключевые)
- **C++20 + Direct2D 1.1 + DirectWrite + WIC** — нативная производительность, контроль памяти
- **WARP device для эффектов** — работает на любой машине, не требует дискретной видеокарты
- **Effect chain через D2D built-in effects** — 90% результата Liquid Glass с 10% сложности custom HLSL
- **Событийная модель окон** — `RegisterShellHookWindow` + `SetWinEventHook` вместо polling
- **Texture Atlas** — снижает draw calls, shelf packing algorithm
- **Watchdog как отдельный процесс** — гарантирует fallback при краше основного

---

## Технологический стек
| Компонент | Технология |
|-----------|-----------|
| Язык | C++20 |
| Сборка | CMake + MSVC |
| Рендеринг | Direct2D 1.1 + DirectWrite + WIC |
| Оффскрин эффекты | D3D11 WARP + ID2D1DeviceContext |
| Окно настроек | Win32 + Direct2D (или WinUI 3) |
| Конфиг | JSON (nlohmann/json — заготовка) |
| Анимации | Собственный tweening-движок |
| Плагины | DLL с C-интерфейсом |
| Инсталлятор | Inno Setup |
| Обновления | WinSparkle / собственный OTA |

---

*Последнее обновление: Chat 03 | Следующий: Chat 04 — Анимационный движок*
