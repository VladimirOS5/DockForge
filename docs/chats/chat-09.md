# Chat 09 — Мульти-мониторы и профили производительности

## Цель

Dock на каждом мониторе, профили Eco/Balanced/Performance/Custom, автопауза в полноэкране/idle.

## Что реализовано

### MonitorManager (src/Core/MonitorManager)

- **EnumDisplayMonitors** — сбор всех мониторов с `MONITORINFOEX`
- **Primary sort** — primary монитор всегда первый
- **Multi-monitor mode** — `primary` (только на главном) или `all` (на каждом)
- **Отдельное окно Dock на каждый монитор** — своё позиционирование через `MonitorInfo::workArea`
- **UpdateAll/RenderAll** — централизованный loop

### PerformanceProfileManager (src/Utils/PerformanceProfile)

- **4 профиля**:
  - Eco: 30 FPS, solid bg, анимации выключены, preview выключены
  - Balanced: 60 FPS, acrylic, все фичи включены
  - Performance: 144 FPS, liquid glass, без адаптивного FPS
  - Custom: не трогает настройки
- **Auto-detect placeholder** — можно расширить на проверку питания от батареи
- **Fullscreen detection** — `GetForegroundWindow` + сравнение с `rcMonitor`
- **Idle detection** — `GetLastInputInfo`, порог 5 минут
- **Pause rendering** — `Sleep(100)` вместо рендера при fullscreen/idle

### Application (обновлён)

- **Убран `m_dockWindow`** — теперь управление через `MonitorManager`
- **Единый message loop** — `PeekMessage` + `UpdateAll` + `RenderAll`
- **Performance check** — перед каждым кадром проверяется `ShouldPauseRendering()`

### DockWindow (обновлён)

- **`Create(HINSTANCE, const MonitorInfo*)`** — привязка к конкретному монитору
- **`Update(deltaTime)` + `Render()`** — публичные методы для внешнего loop
- **`RunMessageLoop()`** — оставлен для совместимости, но не используется

### SettingsWindow (обновлён)

- **Performance Profile** — dropdown (Eco/Balanced/Performance/Custom)
- **Multi-Monitor** — dropdown (Primary Only / All Monitors)
- Применение профиля мгновенное, multi-monitor требует перезапуска

## Сборка

```bash
cd build
cmake .. -A x64
cmake --build . --config Release
```
