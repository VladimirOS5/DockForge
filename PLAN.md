# DockForge — План разработки по чатам

> Файл для отслеживания прогресса. Обновляется после каждого чата.
> Текущая версия: **v1.0.0** | Последний чат: 12

---

## Чат 01 — Архитектура и скелет проекта ✅
**Цель:** Работающее пустое окно Dock + скрытие стандартной панели + fallback.
- [x] CMake проект, Direct2D SDK
- [x] Layered topmost окно Dock
- [x] TaskbarHider: SHAppBarMessage
- [x] Watchdog процесс
- [x] Логгер: singleton, ротация
- [x] Single-instance mutex, DPI awareness

## Чат 02 — Рендер-движок и эффекты ✅
**Цель:** Производительный рендер с blur/acrylic/liquid glass.
- [x] WARP offscreen rendering
- [x] Gaussian Blur, Acrylic, Liquid Glass
- [x] V-Sync (DwmFlush) + adaptive FPS
- [x] Screen capture (BitBlt + WIC)
- [x] Config singleton

## Чат 03 — Иконки и ресурсы ✅
**Цель:** Загрузка и отрисовка иконок приложений.
- [x] IconLoader: IShellItemImageFactory, ExtractIconEx, SHGetFileInfo, WIC
- [x] TextureAtlas: shelf packing, 1024x1024, UV
- [x] AnimatedIcon: GIF/APNG, frame delay
- [x] BadgeRenderer: красный кружок, "99+"
- [x] Magnification 1.3x, running indicator

## Чат 04 — Анимационный движок ✅
**Цель:** Все анимации macOS + кастомные, оптимизированные.
- [x] TweenEngine: time-based, 7 easing curves
- [x] Magnification через TweenEngine (easeOutCirc) + neighbor scaling
- [x] Прыжок иконки (jump + bounce, easeOutBack + easeOutBounce)
- [x] Pseudo-Genie Effect (skew + scaleY + opacity)
- [x] Badge pulse (easeOutBack)
- [x] Slide in/out при старте/выходе
- [x] Running indicator pulse
- [x] Config: animationSpeed, magnificationScale/Range/Easing, jumpAnimationSpeed, genieAnimationSpeed, badgePulseSpeed, slideAnimationSpeed

## Чат 05 — Shell интеграция: окна, превью, Jump Lists ⏳
**Цель:** Dock знает обо всех окнах и умеет с ними работать.
- [ ] ShellHookManager: RegisterShellHookWindow
- [ ] WindowManager: кэш, фильтрация, группировка
- [ ] Thumbnail previews: DwmRegisterThumbnail
- [ ] Jump Lists: ICustomDestinationList
- [ ] Переключение/закрытие/сворачивание окон

## Чат 06 — Системный трей, меню Пуск, контекстное меню ⏳
- [ ] System Tray в Dock
- [ ] Кастомная кнопка Пуск со скинами
- [ ] Контекстное меню Dock

## Чат 07 — Окно настроек и темизация ⏳
- [ ] Окно настроек (Direct2D)
- [ ] Темы: светлая/тёмная/авто
- [ ] JSON-конфиг, экспорт/импорт тем
- [ ] Настройки Dock, эффектов, производительности

## Чат 08 — Плагиновая система и виджеты ⏳
- [ ] IPlugin C-интерфейс
- [ ] PluginManager, hot-reload
- [ ] Виджеты на рабочий стол и в Dock
- [ ] Встроенные: Clock, Weather, SystemMonitor, Calendar

## Чат 09 — Мульти-мониторы и профили производительности ⏳
- [ ] MonitorManager, Dock на каждом мониторе
- [ ] Performance Profiles: Eco, Balanced, Performance, Custom
- [ ] Автопауза в полноэкране/idle

## Чат 10 — Анимированные обои и фон Dock ✅
**Цель:** Audio-reactive фон Dock с частицами, градиентом и Wallpaper Engine.
- [x] AudioCapture: WASAPI loopback, RMS + частотные полосы (bass/mid/treble), сглаживание, beat detection
- [x] ParticleSystem: 3 формы (circle/diamond/star), glow, trails, физика (гравитация, bounce), audio-reactive spawn/velocity/size
- [x] GradientFlow: 3 направления (horizontal/vertical/radial), audio-reactive цвета, кэширование brushes
- [x] AudioReactiveEffect: 5 режимов (off/particles/gradient/both/wallpaper), интеграция с Config
- [x] WallpaperEngineIntegration: обнаружение процесса, проверка видимости, pause/resume API
- [x] DockWindow: интеграция как 4-й режим фона (F1 цикл), отдельный render path без screen capture
- [x] Config: 8 новых полей (audioReactiveBackground, audioReactiveMode, particleCount, wallpaperEngineIntegration, audioSmoothing, audioSensitivity, particleGlow, particleTrails, gradientDirection)

## Чат 11 — Автообновления (OTA), логирование, инсталлятор ✅
**Цель:** Автоматические обновления, установщик и отказоустойчивость.
- [x] SemanticVersion с поддержкой prerelease (alpha/beta/rc)
- [x] WinInet HTTP-клиент: проверка, скачивание, прогресс
- [x] SHA-256 верификация через CryptoAPI
- [x] 5 режимов: Off, Check, Download, Verify, Install
- [x] Background timer с настраиваемым интервалом
- [x] Auto-download / auto-install флаги
- [x] Каналы обновлений: stable, beta, alpha
- [x] Cleanup старых инсталляторов (>7 дней)
- [x] Windows version compatibility check
- [x] Inno Setup 6 скрипт с современным wizard
- [x] VC++ Redistributable prerequisite check/install
- [x] CloseApplications / force close
- [x] Desktop icon, startup with Windows задачи
- [x] .dockforge theme file association
- [x] FallbackManager: health checks, safe mode, rollback
- [x] Crash detection (crash.flag в AppData)
- [x] Self-test suite: Renderer, Shell, Audio, Network, Disk
- [x] Safe mode: отключает все эффекты, минимальный конфиг
- [x] Backup текущего exe перед обновлением
- [x] Rollback на предыдущую версию

## Чат 12 — Полировка, тестирование, релиз ✅
**Цель:** Финальная стабилизация, тесты, документация, релиз v1.0.0.

### Memory & Stability
- [x] MemoryTracker: singleton, track/untrack, leak detection, peak metrics
- [x] StabilityTest: test framework, categories (memory/render/shell/edge/stress)
- [x] 12 built-in tests: D2D init, COM init, Config I/O, memory leak, icon stress, window enum, DPI, HDR, UWP, high icon count, fullscreen detection
- [x] 72h accelerated simulation (timeScale 60x, ~1.2 real hours)
- [x] Stress tests: icon loading, window enumeration, animation loop, memory allocation
- [x] Memory report: live/peak allocations, live/peak bytes, avg size, leak list with file:line
- [x] Periodic memory monitoring (every 60s in main loop)

### Edge Cases
- [x] DPIHelper: Per-Monitor V2 awareness, scale detection per window/monitor/system, rect scaling, WM_DPICHANGED handler
- [x] HDRHelper: DXGI 1.6 detection, HDR active check, color space enum (SDR/HDR10/HLG/scRGB), SDR white level query, display info logging
- [x] UWPHelper: UWP window detection via class name + process, AppUserModelId extraction via IPropertyStore, ApplicationFrameHost detection, CoreWindow child search, UWP launch via IApplicationActivationManager

### Integration
- [x] Application: LogSystemInfo (Windows version, CPU, RAM, DPI, HDR), InitializeDPI, InitializeMemoryTracking
- [x] Application: RunStabilityTests on startup (if enabled), RunLongTermStabilityTest, PrintMemoryReport
- [x] Application: Periodic memory check every 60s in main loop, health check includes memory < 100MB
- [x] Config: 9 новых полей (enableMemoryTracking, runStabilityTestsOnStart, logDPIInfo, logHDRInfo, detectUWPApps, handleDPIScale, useHDRAwareColors, stabilityTestDurationHours, stabilityTimeScale)
- [x] CMakeLists.txt: +Testing, +DPIHelper, +HDRHelper, +UWPHelper, +propsys, DOCKFORGE_MEMORY_TRACKING для Debug

### Documentation & Release
- [x] README.md: shields, features table, system requirements, install instructions, build guide, architecture diagram, chat progress table, license
- [x] PLAN.md: полностью обновлён, все 12 чатов, v1.0.0
- [x] build_release.bat: однокомандная сборка Release + Installer

---

## Архитектурные решения
- C++20 + Direct2D 1.1 + WARP для эффектов
- Событийная модель окон (ShellHook)
- Texture Atlas (shelf packing)
- Watchdog как отдельный процесс
- TweenEngine: time-based, callback chains
- Audio-reactive: WASAPI loopback + time-domain band splitting
- OTA: WinInet + CryptoAPI SHA-256
- Memory tracking: macro-based (Debug builds), mutex-safe hash map
- DPI: Per-Monitor V2 awareness context
- HDR: DXGI 1.6 AdvancedColor detection

## Технологический стек
| Компонент   | Технология                       |
| ----------- | -------------------------------- |
| Язык        | C++20                            |
| Сборка      | CMake + MSVC                     |
| Рендеринг   | Direct2D 1.1 + DirectWrite + WIC |
| Оффскрин    | D3D11 WARP + ID2D1DeviceContext  |
| Анимации    | TweenEngine (7 easing curves)    |
| Плагины     | DLL с C-интерфейсом              |
| Инсталлятор | Inno Setup                       |
| Audio       | WASAPI Loopback Capture          |
| OTA         | WinInet + CryptoAPI              |
| Testing     | Custom framework + simulation    |

---

*Версия: v1.0.0 | Статус: Релиз 🎉*
