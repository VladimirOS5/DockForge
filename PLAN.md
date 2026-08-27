# DockForge — План разработки по чатам

&gt; Файл для отслеживания прогресса. Обновляется после каждого чата.
&gt; Текущая версия: v1.0.0-alpha | Последний чат: 10

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

## Чат 11 — Автообновления (OTA), логирование, инсталлятор ⏳

- [ ] OTA Updater
- [ ] Inno Setup инсталлятор
- [ ] Отказоустойчивость, тесты fallback

## Чат 12 — Полировка, тестирование, релиз ⏳

- [ ] Memory leak detection, 72h stability test
- [ ] Edge cases: UWP, DPI scaling, HDR
- [ ] Документация, README с гифками
- [ ] v1.0.0 release

---

## Архитектурные решения

- C++20 + Direct2D 1.1 + WARP для эффектов
- Событийная модель окон (ShellHook)
- Texture Atlas (shelf packing)
- Watchdog как отдельный процесс
- TweenEngine: time-based, callback chains
- Audio-reactive: WASAPI loopback + time-domain band splitting (лёгкая альтернатива FFT)

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

---

_Последнее обновление: Chat 10 | Следующий: Chat 11 — OTA, инсталлятор_
