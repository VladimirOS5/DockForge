# Chat 07 — Окно настроек и темизация

## Цель

Окно настроек на Direct2D, светлая/тёмная/авто темы, JSON-конфиг, экспорт/импорт тем.

## Что реализовано

### ThemeManager (src/Utils/Theme)

- **3 режима**: Light, Dark, Auto
- **Auto** — читает реестр `AppsUseLightTheme` из `HKEY_CURRENT_USER\...\Personalize`
- **ThemeColors struct** — 11 цветов: background, accent, text, controlBg, border и др.
- **Refresh()** — переоценка Auto при необходимости

### JSON Config (src/Utils/Config)

- **nlohmann/json** (single-header) — сериализация/десериализация
- **Путь**: `%LOCALAPPDATA%\DockForge\config.json`
- **LoadFromFile()** — загрузка при старте, fallback на defaults
- **SaveToFile()** — автосохранение при каждом изменении в Settings
- **ExportTheme() / ImportTheme()** — JSON-файлы тем (type: DockForgeTheme)

### SettingsWindow (src/Settings/SettingsWindow)

- **Direct2D UI** — кастомное окно с табами, тогглами, слайдерами, дропдаунами, кнопками
- **5 категорий**: General, Appearance, Animation, Performance, About
- **Контролы**:
  - Toggle: переключатель с анимацией (визуально)
  - Slider: drag по track, snap к step
  - Dropdown: циклический выбор (click)
  - Button: callback (Export/Import Theme)
- **Цвета** — полностью из ThemeManager
- **Открытие** — через DockContextMenu → Settings...

### Интеграция

- **Application** — `Config::LoadFromFile()` и `ThemeManager::Refresh()` при старте
- **DockContextMenu** — пункт Settings открывает/закрывает окно
- **DockWindow** — FPS и fallback background используют тему
- **Auto-save** — любое изменение в SettingsWindow вызывает `Config::SaveToFile()`

## Сборка

```bash
# 1. Скачай json.hpp:
# https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp
# Положи в third_party/nlohmann/json.hpp

cd build
cmake .. -A x64
cmake --build . --config Release
```
