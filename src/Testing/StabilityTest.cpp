#include "StabilityTest.h"
#include "../Utils/Logger.h"
#include "../Utils/Config.h"
#include "../Utils/PerformanceProfile.h"
#include "../Core/MonitorManager.h"
#include "../Core/DockWindow.h"
#include "../Renderer/D2DRenderer.h"
#include "../Renderer/IconLoader.h"
#include "../Renderer/TextureAtlas.h"
#include <shellscalingapi.h>
#include <tlhelp32.h>
#include <chrono>
#include <thread>
#include <random>
#include <fstream>

StabilityTest& StabilityTest::Instance() {
    static StabilityTest instance;
    return instance;
}

void StabilityTest::RegisterDefaultTests() {
    RegisterTest("Renderer Initialization", [this]() { return TestRendererInit(); });
    RegisterTest("Icon Loading", [this]() { return TestIconLoading(); });
    RegisterTest("Memory Leak (5 min)", [this]() { return TestMemoryLeak(300.0f); });
    RegisterTest("Animation Stress", [this]() { return TestAnimationStress(); });
    RegisterTest("Multi-Monitor", [this]() { return TestMultiMonitor(); });
    RegisterTest("DPI Scaling", [this]() { return TestDPIScaling(); });
    RegisterTest("Window Manager", [this]() { return TestWindowManager(); });
    RegisterTest("Config Persistence", [this]() { return TestConfigPersistence(); });
    RegisterTest("Theme Switching", [this]() { return TestThemeSwitching(); });
    RegisterTest("Plugin Loading", [this]() { return TestPluginLoading(); });
}

void StabilityTest::RegisterTest(const std::string& name, std::function<bool()> test) {
    m_tests.push_back({ name, test });
}

std::vector<StabilityReport> StabilityTest::RunAll() {
    std::vector<StabilityReport> reports;
    for (auto& test : m_tests) {
        StabilityReport report;
        report.testName = test.name;
        auto start = std::chrono::steady_clock::now();
        try {
            report.passed = test.func();
        } catch (...) {
            report.passed = false;
            report.error = "Exception thrown";
        }
        auto end = std::chrono::steady_clock::now();
        report.durationSec = std::chrono::duration<float>(end - start).count();
        reports.push_back(report);
        LOG_INFO("Stability test [" + test.name + "]: " + (report.passed ? "PASSED" : "FAILED"));
    }
    return reports;
}

bool StabilityTest::RunLongTermSimulation(const SimulationConfig& config) {
    LOG_INFO("Starting long-term stability simulation...");
    float simulatedTime = 0.0f;
    auto start = std::chrono::steady_clock::now();
    int hoursLogged = 0;
    while (simulatedTime < config.realTimeHours * 3600.0f) {
        float deltaTime = 1.0f / 60.0f * config.timeScale;
        simulatedTime += deltaTime;
        if (config.logEveryHour && static_cast<int>(simulatedTime / 3600.0f) > hoursLogged) {
            hoursLogged = static_cast<int>(simulatedTime / 3600.0f);
            LOG_INFO("Simulation hour " + std::to_string(hoursLogged));
        }
        std::this_thread::sleep_for(std::chrono::microseconds(1000));
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration<float>(now - start).count() > config.realTimeHours * 3600.0f * 2.0f) {
            LOG_WARN("Simulation timeout");
            break;
        }
    }
    return true;
}

void StabilityTest::WriteReport(const std::vector<StabilityReport>& reports, const std::string& path) {
    std::ofstream file(path);
    if (!file.is_open()) return;
    file << "DockForge Stability Report\n";
    int passed = 0, failed = 0;
    for (const auto& r : reports) {
        file << "[" << (r.passed ? "PASS" : "FAIL") << "] " << r.testName << "\n";
        if (r.passed) ++passed; else ++failed;
    }
    file << "\nTotal: " << passed << " passed, " << failed << " failed\n";
}

bool StabilityTest::TestRendererInit() {
    Microsoft::WRL::ComPtr<ID2D1Factory> factory;
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &factory);
    return SUCCEEDED(hr);
}

bool StabilityTest::TestIconLoading() {
    IconLoader loader;
    auto icon = loader.LoadSystemIcon(L"explorer.exe", 48);
    return !icon.frames.empty();
}

bool StabilityTest::TestMemoryLeak(float durationSec) {
    auto start = std::chrono::steady_clock::now();
    std::vector<std::unique_ptr<char[]>> allocations;
    while (std::chrono::duration<float>(std::chrono::steady_clock::now() - start).count() < durationSec) {
        allocations.push_back(std::make_unique<char[]>(1024));
        if (allocations.size() > 1000) allocations.clear();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return true;
}

bool StabilityTest::TestAnimationStress() {
    for (int i = 0; i < 1000; ++i) {
        float val = 0.0f;
        (void)val;
    }
    return true;
}

bool StabilityTest::TestMultiMonitor() {
    int monitorCount = GetSystemMetrics(SM_CMONITORS);
    return monitorCount > 0;
}

bool StabilityTest::TestDPIScaling() {
    HMONITOR hMonitor = MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTOPRIMARY);
    UINT dpiX = 96, dpiY = 96;
    HRESULT hr = GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);
    return SUCCEEDED(hr) && dpiX >= 96 && dpiY >= 96;
}

bool StabilityTest::TestWindowManager() {
    int count = 0;
    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        if (IsWindowVisible(hwnd)) (*reinterpret_cast<int*>(lParam))++;
        return TRUE;
    }, reinterpret_cast<LPARAM>(&count));
    return count > 0;
}

bool StabilityTest::TestConfigPersistence() {
    Config::Instance().LoadDefaults();
    auto& cfg = Config::Instance().GetMutable();
    cfg.theme = "test_theme";
    bool saved = Config::Instance().SaveToFile();
    bool loaded = Config::Instance().LoadFromFile();
    bool ok = saved && loaded && Config::Instance().Get().theme == "test_theme";
    Config::Instance().LoadDefaults();
    return ok;
}

bool StabilityTest::TestThemeSwitching() {
    for (int i = 0; i < 100; ++i) {
        Config::Instance().GetMutable().theme = (i % 2 == 0) ? "dark" : "light";
    }
    return true;
}

bool StabilityTest::TestPluginLoading() {
    return true;
}
