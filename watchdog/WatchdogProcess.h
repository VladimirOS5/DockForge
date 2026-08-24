#pragma once
#include <windows.h>
#include <string>

class WatchdogProcess {
public:
    WatchdogProcess();
    ~WatchdogProcess();
    bool StartMonitoring(const std::wstring& targetProcessName);
    void Stop();
    void Run();
    bool IsRunning() const { return m_running; }
private:
    bool IsTargetRunning();
    void RestoreTaskbar();
    bool RestartTarget();
    std::wstring GetTargetPath();
    std::wstring m_targetName;
    HANDLE m_stopEvent = nullptr;
    bool m_running = false;
    int m_restartAttempts = 0;
    static constexpr int MAX_RESTART_ATTEMPTS = 3;
};
