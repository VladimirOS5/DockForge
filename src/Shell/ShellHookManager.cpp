#include "ShellHookManager.h"
#include "../Utils/Logger.h"

ShellHookManager& ShellHookManager::Instance() {
    static ShellHookManager instance;
    return instance;
}
bool ShellHookManager::Initialize() {
    m_initialized = true;
    LOG_INFO("ShellHookManager initialized (stub)");
    return true;
}
void ShellHookManager::Shutdown() { m_initialized = false; }
bool ShellHookManager::IsInitialized() const { return m_initialized; }
void ShellHookManager::SetCallback(std::function<void(const ShellEvent&)> cb) { m_callback = cb; }
