# Chat 03 — Иконки и ресурсы

## Цель
Загрузка реальных иконок приложений, текстурный атлас, анимированные иконки (GIF/APNG), badge-уведомления.

## Что реализовано

### IconLoader
- **IShellItemImageFactory** — высококачественные иконки для UWP/Modern приложений, поддержка любого размера.
- **ExtractIconExW** — fallback для классических Win32 .exe.
- **SHGetFileInfo** — крайний fallback, системные иконки.
- **WIC декодер** — PNG, JPEG, BMP, GIF, APNG → `ID2D1Bitmap`.
- **HICON → D2D1Bitmap** — ручная конвертация через DIBSection с premultiplied alpha.
- **Масштабирование** — сохранение aspect ratio, high-quality cubic interpolation через WIC scaler.

### TextureAtlas
- **Shelf packing algorithm** — простой и быстрый алгоритм упаковки: полки слева направо, новая полка при переполнении.
- **1024×1024 atlas** — все статичные иконки (первый кадр анимаций) упакованы в одну GPU-текстуру.
- **UV-координаты** — каждая иконка хранит `u0, v0, u1, v1` для отрисовки через `DrawBitmapFromAtlas`.
- **2px padding** — между иконками для предотвращения bleeding при фильтрации.
- **Auto-rebuild** — при переполнении атласа автоматически очищается и перестраивается.

### AnimatedIcon
- **Кадровая анимация** — хранит вектор `ID2D1Bitmap` для каждого кадра.
- **GIF frame delay** — читается из WIC metadata (`/grctlext/Delay`), конвертируется из 1/100с в мс.
- **Time-based playback** — `std::chrono::steady_clock`, не зависит от FPS.
- **Loop** — циклическое воспроизведение.

### BadgeRenderer
- **Красный кружок** с белой обводкой (1.5px).
- **Цифра внутри** — Segoe UI Bold 10pt, центрирована.
- **"99+"** — при badge > 99 показывается "99+".
- **Позиционирование** — верхний правый угол иконки, частично выходит за границы.

### DockWindow (обновлён)
- **DockIcon struct** — полное описание иконки: путь, загруженная иконка, аниматор, atlas entry, badge, scale.
- **LoadDemoIcons()** — загружает 5 системных иконок (Explorer, Notepad, Calc, Edge, Settings) с разными badge.
- **Magnification** — при наведении масштаб плавно интерполируется к 1.3x (speed 10.0f).
- **Running indicator** — белая точка 4×4 px под иконкой: яркая для запущенных, тусклая для pinned.
- **Отрисовка** — shadow → atlas bitmap → badge → indicator.

### D2DRenderer (обновлён)
- **DrawBitmapFromAtlas()** — рисует под-регион атласа по UV-координатам. Использует `srcRect` в `DrawBitmap`.

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
- **F1** — переключить эффект
- **Наведение мыши** — magnification 1.3x
- **Badge** — красные кружки с цифрами на Edge (5) и Explorer (3)

## Архитектурные решения
- **Atlas только для первого кадра** — анимированные иконки рисуются напрямую (не через atlas), т.к. обновление atlas каждый кадр дорого. В Чат 04 добавим multi-frame atlas для анимаций.
- **IShellItemImageFactory приоритет** — даёт лучшее качество, чем ExtractIconEx, особенно для UWP. Поддерживает размеры > 256px.
- **Premultiplied alpha** — при конвертации HICON → D2D1Bitmap вручную умножаем RGB на A. Direct2D требует premultiplied для корректного blending.
- **WIC high-quality cubic** — лучшее качество downscale иконок, чем nearest neighbor.

## Следующий чат
**Чат 04 — Анимационный движок**: tween engine, easing curves, прыжок иконки, genie effect, настраиваемые параметры.
