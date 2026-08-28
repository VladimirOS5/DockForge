#pragma once
#include <windows.h>
#include <atomic>
#include <memory>

class TaskbarHider;

class Application {
public:
    Application();
    ~Application();
    bool Initialize(HINSTANCE hInstance);
    int Run();
    void Shutdown();
    void RequestQuit() { m_running = false; }
    void RequestRestart() { m_restartRequested = true; m_running = false; }
    bool IsRestartRequested() const { return m_restartRequested; }
    void InstallPendingUpdate();
    void RunStabilityTests();
    void PrintMemoryReport();
private:
    void InitializeOTA();
    void InitializeFallback();
    void RunSelfTests();
    void UpdateOTA(float deltaTime);
    void ShowUpdateNotification();
    void InitializeMemoryTracking();
    void InitializeDPI();
    void LogSystemInfo();
    void RunLongTermStabilityTest();

    HINSTANCE m_hInstance = nullptr;
    bool m_initialized = false;
    std::atomic<bool> m_running{true};
    bool m_restartRequested = false;
    bool m_updatePending = false;
    float m_otaTimer = 0.0f;
    std::unique_ptr<TaskbarHider> m_taskbarHider;
};
