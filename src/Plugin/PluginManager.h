#pragma once
#include "IPlugin.h"
#include <string>
#include <vector>
#include <memory>
#include <filesystem>

struct WidgetBase {
    virtual ~WidgetBase() = default;
    virtual void Update(float deltaTime) {}
    virtual void Render(ID2D1RenderTarget* rt, IDWriteFactory* wf, float x, float y, float w, float h) = 0;
    virtual void OnClick(float x, float y) {}
    virtual float GetWidth() const { return 64.0f; }
    virtual float GetHeight() const { return 48.0f; }
    virtual const char* GetName() const { return "Widget"; }
};

class PluginManager {
public:
    static PluginManager& Instance();
    void Initialize(ID2D1RenderTarget* rt = nullptr, IDWriteFactory* wf = nullptr, HWND dockHwnd = nullptr);
    void Shutdown();
    void LoadPlugins(const std::wstring& directory);
    void UnloadAll();
    void Update(float deltaTime);
    void Render(ID2D1RenderTarget* rt, IDWriteFactory* wf, float x, float y, float maxHeight);
    void OnClick(float x, float y);
    void CheckHotReload();
    std::vector<WidgetBase*>& GetWidgets() { return m_widgets; }
private:
    PluginManager() = default;
    struct PluginDll {
        HMODULE hMod = nullptr;
        std::filesystem::path path;
        std::filesystem::file_time_type lastWrite;
        DFPluginVTable* vtable = nullptr;
    };
    DFPluginContext m_context;
    std::vector<PluginDll> m_dlls;
    std::vector<std::unique_ptr<WidgetBase>> m_builtin;
    std::vector<WidgetBase*> m_widgets;
    UINT_PTR m_timer = 0;
    static void CALLBACK TimerProc(HWND, UINT, UINT_PTR, DWORD);
};