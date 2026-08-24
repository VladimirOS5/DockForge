# Chat 04 — Анимационный движок

## Цель
Все анимации macOS + кастомные, оптимизированные через tween engine.

## Что реализовано

### TweenEngine (src/Renderer/TweenEngine)
- **Time-based interpolation** — не зависит от FPS, работает по реальному времени.
- **7 easing curves**: Linear, EaseOut, EaseInOut, EaseOutBack, EaseOutBounce, EaseOutCirc, Elastic.
- **String-to-enum** конвертация для конфигурации.
- **Callback onComplete** — цепочка анимаций (jump: up → down, genie: compress → expand).
- **Remove by ID** — отмена предыдущего tween при старте нового.
- **Auto-cleanup** — завершённые tweens удаляются из вектора.

### Config (обновлён)
Добавлены анимационные настройки:
- `animationSpeed` — глобальный множитель скорости (0.5x = медленнее, 2.0x = быстрее)
- `magnificationScale` — максимальный масштаб при наведении (default 1.3x)
- `magnificationRange` — сколько соседних иконок тоже увеличиваются (default 2)
- `magnificationEasing` — тип easing (default "easeOutCirc")
- `jumpAnimation` / `jumpAnimationSpeed` — прыжок иконки
- `genieEffect` / `genieAnimationSpeed` — pseudo-genie эффект
- `badgePulse` / `badgePulseSpeed` — пульсация badge
- `slideAnimationSpeed` — появление Dock при старте
- `runningIndicatorPulse` — пульсация точки-индикатора

### Magnification (обновлён)
- Раньше: ручная интерполяция `currentScale += (target - current) * speed * deltaTime`
- Теперь: **TweenEngine** с `easeOutCirc` — точно как на macOS Dock
- **Neighbor scaling**: иконки в радиусе `magnificationRange` тоже увеличиваются пропорционально расстоянию
- При уходе мыши — tween обратно к 1.0x

### Jump Animation (ЛКМ по иконке)
- **Phase 1** (40% времени): подпрыгивание вверх на 20px с `easeOutBack`
- **Phase 2** (60% времени): падение вниз с `easeOutBounce` (отскок)
- **Scale pulse**: 1.0 → 1.15 → 1.0
- **Opacity**: остаётся 1.0 (можно настроить)

### Genie Effect (двойной ЛКМ по иконке)
- **Pseudo-genie** через Direct2D transforms (без HLSL):
  - `skewX` -0.3 → 0 (наклон)
  - `scaleY` 1.0 → 0.1 → 1.0 (сжатие)
  - `opacity` 1.0 → 0.3 → 1.0 (прозрачность)
- Визуально имитирует "втягивание" в Dock
- Полноценный vertex-shader genie — в Чате 10

### Badge Pulse (ПКМ по иконке)
- Масштаб badge: 1.0 → 1.5 → 1.0
- Easing: `easeOutBack` для "перелёта"
- Демонстрация — в реальности будет при новом уведомлении

### Slide In (при старте)
- Dock появляется снизу: `offsetY` от `height + margin` до 0
- `opacity` 0 → 1
- Easing: `defaultEasing` (easeOutBack)
- Длительность: `slideAnimationSpeed` (default 600ms)

### Running Indicator Pulse
- Точка под иконкой пульсирует: `opacity = 0.7 + 0.3 * sin(time * 3)`
- Работает для pinned (тускло) и running (ярко)

### DockWindow (обновлён)
- Все анимированные свойства вынесены в `DockIcon` struct
- `SetIconHover()` — управление magnification tweens
- `TriggerJump()` / `TriggerGenie()` / `TriggerBadgePulse()` — запуск анимаций
- `UpdateAnimations()` — вызов `TweenEngine::Update()` + indicator pulse
- Обработка мыши: `WM_MOUSEMOVE`, `WM_LBUTTONDOWN`, `WM_LBUTTONDBLCLK`, `WM_RBUTTONDOWN`
- `m_slideOffsetY` / `m_slideOpacity` — глобальные tweened свойства Dock

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

**Управление:**
- **F1** — переключить эффект фона
- **Наведение** — magnification соседних иконок (easeOutCirc)
- **ЛКМ** — прыжок иконки (jump + bounce)
- **Двойной ЛКМ** — genie effect (pseudo)
- **ПКМ** — badge pulse

## Архитектурные решения
- **TweenEngine вместо ручной интерполяции** — проще, надёжнее, настраиваемее. Каждая анимация — независимый tween с собственным easing.
- **Callback chains** — прыжок и genie реализованы как последовательность tweens через `onComplete`. Это позволяет комбинировать анимации без сложных state machines.
- **Tween ID tracking** — каждая иконка хранит `tweenIdScale`, `tweenIdJump` и т.д. Это позволяет отменять предыдущий tween при старте нового (например, быстрое наведение/уход мыши).
- **Time-based, не frame-based** — анимации работают корректно при любом FPS, включая adaptive mode.
- **Pseudo-genie вместо mesh deformation** — честный genie требует vertex shader (HLSL custom effect). В Чате 10 добавим полноценный вариант, пока pseudo-genie даёт 80% визуального эффекта с 5% сложности.

## Следующий чат
**Чат 05 — Shell интеграция**: окна, thumbnail previews, Jump Lists, группировка.
