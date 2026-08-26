# Chat 08 — Плагиновая система и виджеты

## Цель

IPlugin C-интерфейс, PluginManager с hot-reload, встроенные виджеты (Clock, SystemMonitor), заготовка DesktopWidget.

## Что реализовано

### IPlugin (src/Plugin/IPlugin)

- **C-интерфейс (ABI-stable)** — `DFPluginVTable` с function pointers
- **Версионирование** — `apiVersion` для совместимости
- **Контекст** — renderTarget, writeFactory, dockHwnd передаются плагину
- **Экспортная функция** — `GetDockForgePlugin()` с манглингом C

### PluginManager (src/Plugin/PluginManager)

- **Загрузка DLL** — сканирование `plugins/*.dll`, `LoadLibrary` + `GetProcAddress`
- **Built-in widgets** — Clock, SystemMonitor (статически слинкованы, тот же интерфейс)
- **Hot-reload** — таймер на 3 секунды, отслеживание `last_write_time`, unload/load
- **Единый список** — `m_widgets` содержит raw ptr на все виджеты (built-in + DLL)

### ClockWidget (BuiltIn)

- **Обновление** — каждую секунду через `localtime_s`
- **Рендер** — DirectWrite, центрирование, Segoe UI 16pt
- **Размер** — 70x48

### SystemMonitorWidget (BuiltIn)

- **CPU usage** — `GetSystemTimes` (idle/kernel/user), расчёт delta
- **Визуализация** — вертикальный бар с цветовой индикацией (зелёный→жёлтый→красный)
- **Обновление** — раз в секунду

### DesktopWidgetWindow (src/Widget)

- **Заготовка** — layered окно для виджетов на рабочем столе
- **WS_EX_NOACTIVATE** — не перехватывает фокус
- **TODO** — D2D рендеринг (требует отдельного device context)

### Интеграция в Dock

- **Рендер** — виджеты рисуются справа от tray-иконок
- **Update** — вызывается в `RunMessageLoop`
- **PluginManager::Shutdown** — вызывается перед уничтожением DockWindow

## Как создать внешний плагин

```cpp
#include "IPlugin.h"
#include &lt;wrl/client.h&gt;

static DFPluginVTable g_vtable;

static bool MyInit(DFPluginContext*) { return true; }
static void MyRender(ID2D1RenderTarget* rt, IDWriteFactory*, float x, float y, float w, float h) {
    Microsoft::WRL::ComPtr&lt;ID2D1SolidColorBrush&gt; b;
    rt-&gt;CreateSolidColorBrush(D2D1::ColorF(1,0,0), &b);
    rt-&gt;FillRectangle(D2D1::RectF(x,y,x+w,y+h), b.Get());
}

extern "C" __declspec(dllexport) DFPluginVTable* GetDockForgePlugin() {
    g_vtable = { DF_PLUGIN_API_VERSION, "MyPlugin", "1.0", MyInit, nullptr, nullptr, MyRender, nullptr, 64, 48 };
    return &g_vtable;
}
```
