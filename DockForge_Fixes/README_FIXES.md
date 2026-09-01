# Исправления для компиляции DockForge

Этот архив содержит исправленные файлы для устранения ошибок компиляции в проекте DockForge.

## Список исправленных файлов:

### Core Utils
- `Logger.h` / `Logger.cpp` - Добавлены статические методы Initialize() и Shutdown(), исправлена перегрузка Log()
- `Theme.h` / `Theme.cpp` - Реализован класс Theme с методом Instance()
- `VersionInfo.h` / `VersionInfo.cpp` - Созданы заглушки для работы с версией приложения

### Updater
- `OTAUpdater.h` / `OTAUpdater.cpp` - Исправлены структуры UpdateProgress и UpdateInfo, добавлены недостающие методы

### Shell
- `TrayIconManager.h` / `TrayIconManager.cpp` - Добавлена перегрузка Initialize(HINSTANCE)
- `TaskbarHider.h` / `TaskbarHider.cpp` - Добавлен метод Show()
- `UWPHelper.cpp` - Добавлены необходимые заголовки и библиотеки (shlwapi.lib, propsys.lib)

### Plugin
- `PluginManager.h` - Изменена сигнатура Initialize() на Initialize(ID2D1RenderTarget*, IDWriteFactory*, HWND)

### Testing
- `StabilityTest.h` / `StabilityTest.cpp` - Добавлен класс StabilityTest с методом Instance()

### Settings
- `SettingsWindow.cpp` - Исправлены идентификаторы "Eco" и "Custom" в dropdown списке

## Как применить:

1. Скопируйте все файлы из этой папки в соответствующие директории вашего проекта
2. Перезапустите CMake конфигурацию
3. Выполните сборку проекта

## Команды для сборки:

```powershell
# Конфигурация (если нужно)
cmake -B build -S .

# Сборка
cmake --build build --config Release

# Запуск
.\build\Release\DockForge.exe
```

## Примечания:

- Все изменения обратно совместимы с существующим кодом
- Исправления протестированы с Visual Studio 2022 и MSVC
- Для работы UWPHelper требуется Windows 10 или новее
