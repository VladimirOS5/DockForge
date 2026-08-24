# DockForge

Панель задач для Windows 11, вдохновлённая macOS Dock, но лучше и функциональнее.

## Статус
🚧 Альфа-разработка — Чат 04 завершён (анимационный движок)

## Требования
- Windows 11
- Visual Studio 2022 (или MSVC build tools)
- CMake 3.20+
- Windows SDK 10.0.22000.0+

## Сборка

```bash
git clone <repo>
cd DockForge
mkdir build && cd build
cmake .. -A x64
cmake --build . --config Release
```

## Запуск
```bash
# Опционально: watchdog
.\watchdog\Release\DockForge.Watchdog.exe

# Основное приложение
.\src\Release\DockForge.exe
```

**Управление:**
- **F1** — переключить эффект фона (Acrylic / Liquid Glass / Solid)
- **Наведение** — magnification иконок с easing (как на macOS)
- **ЛКМ** — прыжок иконки (jump + bounce)
- **Двойной ЛКМ** — genie effect
- **ПКМ** — badge pulse

## Архитектура
```
src/
  Core/       — Application, DockWindow, Main
  Shell/      — TaskbarHider
  Renderer/   — D2DRenderer, EffectRenderer, Effects, FrameLimiter,
                IconLoader, TextureAtlas, AnimatedIcon, BadgeRenderer,
                TweenEngine
  Utils/      — Logger, Config, ScreenCapture
docs/chats/   — Логи разработки
PLAN.md       — План по чатам
```

## План разработки
| Чат | Тема | Статус |
|-----|------|--------|
| 01 | Скелет проекта | ✅ |
| 02 | Рендер-движок и эффекты | ✅ |
| 03 | Иконки и ресурсы | ✅ |
| 04 | Анимационный движок | ✅ |
| 05 | Shell интеграция | ⏳ |
| 06 | Трей и меню Пуск | ⏳ |
| 07 | Настройки и темы | ⏳ |
| 08 | Плагины и виджеты | ⏳ |
| 09 | Мульти-мониторы | ⏳ |
| 10 | Анимированные обои | ⏳ |
| 11 | OTA и инсталлятор | ⏳ |
| 12 | Релиз | ⏳ |

## Лицензия
MIT
