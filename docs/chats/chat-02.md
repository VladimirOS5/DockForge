# Chat 02 — Рендер-движок и эффекты

## Цель
Добавить производительный рендер с эффектами: Gaussian Blur, Acrylic, Liquid Glass, адаптивный FPS, V-Sync.

## Что реализовано

### EffectRenderer
- **WARP-based offscreen rendering**: отдельный `ID2D1DeviceContext` через D3D11 WARP device.
- Рендерит эффекты в offscreen bitmap, результат копируется в основной `ID2D1HwndRenderTarget`.
- Это позволяет использовать `ID2D1Effect` (требует D2D 1.1 / DeviceContext) без отказа от layered window.

### Эффекты (src/Renderer/Effects.cpp)
- **BlurEffect** — обёртка над `CLSID_D2D1GaussianBlur`. Настраиваемый `standardDeviation`.
- **AcrylicEffect** — цепочка: `GaussianBlur` → `ColorMatrix` (tint). Noise texture генерируется процедурно (WIC bitmap). Полноценное blend с noise будет доработано в Чат 07.
- **LiquidGlassEffect** — цепочка: `GaussianBlur` (heavy, 20px) → `DisplacementMap` (с анимированной displacement texture) → `Saturation`. Имитирует refraction + volumetric glow.
  - Displacement texture обновляется каждый кадр для анимации "жидкого" эффекта.
  - Настраиваемые: `refractionStrength`, `glowIntensity`, `saturation`.

### FrameLimiter
- **V-Sync**: через `DwmFlush()` — точная синхронизация с compositor Windows.
- **Adaptive FPS**: если кадр рендерится < 2 мс и нет активности — пропуск кадров (`m_skipFrame`).
- **FPS counter**: плавающее среднее за 1 секунду.
- Настройки: `targetFPS`, `vsync`, `adaptiveFPS`, `idleFPS` (заготовка для Чат 09).

### ScreenCapture
- `BitBlt` + WIC конвертация → `ID2D1Bitmap` для offscreen context.
- Захват региона экрана под Dock для использования как input эффектов.
- Fallback на gradient, если `BitBlt` не сработал (DWM-защита, UWP-окна).

### D2DRenderer (обновлён)
- Добавлен `DrawBitmap()` — рисует `ID2D1Bitmap` из EffectRenderer в HWND target.
- Добавлен `DrawTextLayout()` — для отображения FPS и будущих label'ов.
- Подключен DirectWrite (`IDWriteFactory`).

### DockWindow (обновлён)
- **F1** — переключение эффектов: Solid → Acrylic → Liquid Glass.
- `DrawBackgroundEffect()` — рендерит фон через EffectRenderer, применяет выбранный эффект, рисует результат.
- `DrawIcons()` — улучшенные placeholder-иконки с gloss-эффектом.
- `DrawFPS()` — отображение FPS (включается в Config).
- Анимация Liquid Glass обновляется по времени (`deltaTime`).

### Config
- Singleton с `DockConfig` struct.
- Поля: `targetFPS`, `vsync`, `adaptiveFPS`, `backgroundEffect`, `blurRadius`, `tint*`, `refractionStrength`, `glowIntensity`, etc.
- Загрузка/сохранение в JSON — в Чат 07.

## Сборка

```bash
cd build
cmake .. -A x64
cmake --build . --config Release
```

## Запуск
```bash
.\src\Release\DockForge.exe
```
- **F1** — переключить эффект (Acrylic / Liquid Glass / Solid)
- Должна появиться Dock-панель с blur-эффектом фона

## Архитектурные решения
- **WARP device для эффектов**: гарантированно работает на любой машине, не требует дискретной видеокарты. Основной рендер остаётся на `ID2D1HwndRenderTarget` для совместимости с `WS_EX_LAYERED`.
- **Effect chain через D2D built-in effects**: вместо custom HLSL (который требует ~400 строк COM boilerplate) используем `GaussianBlur` + `DisplacementMap` + `Saturation`. Это даёт 90% визуального результата Liquid Glass с 10% сложности. Настоящий HLSL custom shader добавим в Чат 10 для audio-reactive эффектов.
- **Screen capture через BitBlt**: простой и быстрый способ получить фон. Не работает с некоторыми DWM-окнами — заменим на `DwmRegisterThumbnail` в Чат 05.
- **V-Sync через DwmFlush()**: точнее, чем `Sleep()`, и не требует DXGI swap chain.

## Следующий чат
**Чат 03 — Иконки и ресурсы**: загрузка реальных иконок .exe, текстурный атлас, анимированные иконки (GIF/APNG), badge-уведомления.
