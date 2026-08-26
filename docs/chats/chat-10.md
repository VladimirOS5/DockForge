# Chat 10 — Анимированные обои и фон Dock

## Цель

Audio-reactive эффекты, particle system, gradient flow, интеграция с Wallpaper Engine.

## Что реализовано

### AudioCapture (src/Renderer/AudioCapture)

- **WASAPI Loopback** — захват системного аудио через `AUDCLNT_STREAMFLAGS_LOOPBACK`
- **Отдельный поток** — `CaptureThread` читает буфер и считает RMS
- **Форматы** — поддержка IEEE Float и 16-bit PCM
- **RMS level** — 0..1, сглаженное значение для реакции

### ParticleSystem (src/Renderer/ParticleSystem)

- **CPU particles** — position, velocity, life, size, color
- **Audio-reactive spawn** — чем громче музыка, тем больше и быстрее частицы
- **HSV цвета** — цвет меняется со временем и от уровня звука
- **Fade out** — прозрачность зависит от life
- **Max 300 частиц** — настраивается через Config

### GradientFlow (src/Renderer/GradientFlow)

- **Анимированный линейный градиент** — 3 stop'а с синусоидальным смещением цвета
- **Скорость** — 0.15 offset/sec
- **Рендер** — `ID2D1LinearGradientBrush`

### AudioReactiveEffect (src/Renderer/AudioReactiveEffect)

- **Композитор** — объединяет AudioCapture + ParticleSystem + GradientFlow
- **Fallback** — если WASAPI недоступен, gradient flow работает без audio
- **Bounds** — размеры Dock'а передаются перед рендером

### WallpaperEngineIntegration (src/Shell)

- **Detection** — поиск процесса `wallpaper32.exe` / `wallpaper64.exe`
- **Visibility check** — определение WorkerW окна
- **Placeholder** — в будущем можно передавать pause/play команды

### DockWindow (обновлён)

- **AudioReactiveEffect** — инициализируется если включён в Config
- **Render** — частицы рисуются поверх background effect (Acrylic/LiquidGlass)
- **Update** — `m_audioReactive-&gt;Update(deltaTime)` в `UpdateAnimations`

### SettingsWindow (обновлён)

- **Gradient Flow Background** — toggle
- **Audio-Reactive Particles** — toggle
- **Wallpaper Engine Integration** — toggle

## Сборка

```bash
cd build
cmake .. -A x64
cmake --build . --config Release
```
