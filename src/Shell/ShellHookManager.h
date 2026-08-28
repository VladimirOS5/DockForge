#pragma once
#include <windows.h>
#include <functional>

enum class ShellEventType { WindowCreated, WindowDestroyed, WindowActivated, WindowRedraw };
struct ShellEvent { ShellEventType type; HWND hwnd; };

class ShellHookManager {
public:
    static ShellHookManager& Instance();
    bool Initialize();
    void Shutdown();
    bool IsInitialized() const;
    void SetCallback(std::function<void(const ShellEvent&)> cb);
private:
    ShellHookManager() = default;
    bool m_initialized = false;
    std::function<void(const ShellEvent&)> m_callback;
};
