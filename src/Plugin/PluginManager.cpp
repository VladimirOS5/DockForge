#include "PluginManager.h"
#include "BuiltIn/ClockWidget.h"
#include "BuiltIn/SystemMonitorWidget.h"
#include "../Utils/Logger.h"

PluginManager& PluginManager::Instance() {
    static PluginManager instance;
    return instance;
}

void PluginManager::Initialize(ID2D1RenderTarget* rt, IDWriteFactory* wf, HWND dockHwnd) {
    // Compatibility: allow calling with no arguments
    if (!rt || !wf || !dockHwnd) {
        LOG_INFO("PluginManager initialized (deferred)");
        return;
    }
    
    m_context.renderTarget = rt;
    m_context.writeFactory = wf;
    m_context.dockHwnd = dockHwnd;

    auto clock = std::make_unique<ClockWidget>();
    m_widgets.push_back(clock.get());
    m_builtin.push_back(std::move(clock));

    auto monitor = std::make_unique<SystemMonitorWidget>();
    m_widgets.push_back(monitor.get());
    m_builtin.push_back(std::move(monitor));

    LOG_INFO("PluginManager initialized with " + std::to_string(m_widgets.size()) + " built-in widgets");
}

void PluginManager::Shutdown() {
    UnloadAll();
    m_builtin.clear();
    m_widgets.clear();
    LOG_INFO("PluginManager shutdown");
}

void PluginManager::LoadPlugins(const std::wstring& directory) {
    if (!std::filesystem::exists(directory)) {
        std::filesystem::create_directories(directory);
        return;
    }
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.is_regular_file() && entry.path().extension() == L".dll") {
            auto path = entry.path().wstring();
            HMODULE hMod = LoadLibraryW(path.c_str());
            if (!hMod) continue;
            auto getPlugin = (DFGetPluginFunc)GetProcAddress(hMod, "GetDockForgePlugin");
            if (!getPlugin) { FreeLibrary(hMod); continue; }
            DFPluginVTable* vt = getPlugin();
            if (!vt || vt->apiVersion != DF_PLUGIN_API_VERSION) { FreeLibrary(hMod); continue; }
            if (vt->Initialize && !vt->Initialize(&m_context)) { FreeLibrary(hMod); continue; }

            PluginDll p;
            p.hMod = hMod; p.path = entry.path(); p.vtable = vt;
            p.lastWrite = std::filesystem::last_write_time(entry.path());
            m_dlls.push_back(p);

            LOG_INFO(std::string("Loaded plugin: ") + vt->name + " v" + vt->version);
        }
    }
    m_timer = SetTimer(nullptr, 0, 3000, TimerProc);
}

void PluginManager::UnloadAll() {
    for (auto& p : m_dlls) {
        if (p.vtable && p.vtable->Shutdown) p.vtable->Shutdown();
        if (p.hMod) FreeLibrary(p.hMod);
    }
    m_dlls.clear();
    if (m_timer) { KillTimer(nullptr, m_timer); m_timer = 0; }
}

void PluginManager::Update(float deltaTime) {
    for (auto* w : m_widgets) if (w) w->Update(deltaTime);
    for (auto& p : m_dlls) {
        if (p.vtable && p.vtable->Update) p.vtable->Update(deltaTime);
    }
}

void PluginManager::Render(ID2D1RenderTarget* rt, IDWriteFactory* wf, float x, float y, float maxHeight) {
    float cx = x;
    for (auto* w : m_widgets) {
        if (!w) continue;
        float wW = w->GetWidth();
        float wH = std::min(w->GetHeight(), maxHeight);
        w->Render(rt, wf, cx, y + (maxHeight - wH) / 2.0f, wW, wH);
        cx += wW + 8;
    }
}

void PluginManager::OnClick(float x, float y) {
    float cx = 0;
    for (auto* w : m_widgets) {
        if (!w) continue;
        float wW = w->GetWidth();
        if (x >= cx && x < cx + wW) {
            w->OnClick(x - cx, y);
            return;
        }
        cx += wW + 8;
    }
}

void PluginManager::CheckHotReload() {
    for (auto& p : m_dlls) {
        if (!std::filesystem::exists(p.path)) continue;
        auto current = std::filesystem::last_write_time(p.path);
        if (current != p.lastWrite) {
            LOG_INFO(std::string("Hot-reload: ") + p.path.string());
            if (p.vtable && p.vtable->Shutdown) p.vtable->Shutdown();
            if (p.hMod) { FreeLibrary(p.hMod); p.hMod = nullptr; }

            HMODULE hMod = LoadLibraryW(p.path.c_str());
            if (hMod) {
                auto getPlugin = (DFGetPluginFunc)GetProcAddress(hMod, "GetDockForgePlugin");
                if (getPlugin) {
                    DFPluginVTable* vt = getPlugin();
                    if (vt && vt->apiVersion == DF_PLUGIN_API_VERSION && vt->Initialize && vt->Initialize(&m_context)) {
                        p.hMod = hMod; p.vtable = vt; p.lastWrite = current;
                        LOG_INFO(std::string("Hot-reloaded: ") + vt->name);
                        continue;
                    }
                }
                FreeLibrary(hMod);
            }
            LOG_ERROR("Failed to hot-reload plugin");
        }
    }
}

void CALLBACK PluginManager::TimerProc(HWND, UINT, UINT_PTR, DWORD) {
    Instance().CheckHotReload();
}