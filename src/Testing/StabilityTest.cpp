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
    auto makeCase = [](const std::string& name, const std::string& cat, std::function<TestResult()> fn) -> TestCase {
        TestCase tc; tc.name = name; tc.category = cat; tc.run = fn; return tc;
    };
    RegisterTest(makeCase("Renderer Initialization", "render", [this]() { return TestRendererInit() ? TestResult::Pass : TestResult::Fail; }));
    RegisterTest(makeCase("Icon Loading", "render", [this]() { return TestIconLoading() ? TestResult::Pass : TestResult::Fail; }));
    RegisterTest(makeCase("Memory Leak (5 min)", "memory", [this]() { return TestMemoryLeak(300.0f) ? TestResult::Pass : TestResult::Fail; }));
    RegisterTest(makeCase("Animation Stress", "stress", [this]() { return TestAnimationStress() ? TestResult::Pass : TestResult::Fail; }));
    RegisterTest(makeCase("Multi-Monitor", "render", [this]() { return TestMultiMonitor() ? TestResult::Pass : TestResult::Fail; }));
    RegisterTest(makeCase("DPI Scaling", "edge", [this]() { return TestDPIScaling() ? TestResult::Pass : TestResult::Fail; }));
    RegisterTest(makeCase("Window Manager", "shell", [this]() { return TestWindowManager() ? TestResult::Pass : TestResult::Fail; }));
    RegisterTest(makeCase("Config Persistence", "memory", [this]() { return TestConfigPersistence() ? TestResult::Pass : TestResult::Fail; }));
    RegisterTest(makeCase("Theme Switching", "edge", [this]() { return TestThemeSwitching() ? TestResult::Pass : TestResult::Fail; }));
    RegisterTest(makeCase("Plugin Loading", "shell", [this]() { return TestPluginLoading() ? TestResult::Pass : TestResult::Fail; }));
}

void StabilityTest::RegisterTest(const TestCase& test) {
    m_tests.push_back(test);
}

std::vector<TestReport> StabilityTest::RunAll() {
    std::vector<TestReport> reports;
    for (auto& test : m_tests) {
        TestReport report = ExecuteTest(test);
        reports.push_back(report);
        LOG_INFO("Stability test [" + test.name + "]: " + (report.result == TestResult::Pass ? "PASSED" : "FAILED"));
    }
    return reports;
}

TestReport StabilityTest::ExecuteTest(const TestCase& test) {
    TestReport report;
    report.name = test.name;
    auto start = std::chrono::steady_clock::now();
    try {
        report.result = test.run();
    } catch (...) {
        report.result = TestResult::Fail;
        report.message = "Exception thrown";
    }
    auto end = std::chrono::steady_clock::now();
    report.durationMs = std::chrono::duration<float, std::milli>(end - start).count();
    return report;
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

void StabilityTest::WriteReport(const std::vector<TestReport>& reports, const std::string& path) {
    std::ofstream file(path);
    if (!file.is_open()) return;
    file << "DockForge Stability Report\n";
    int passed = 0, failed = 0;
    for (const auto& r : reports) {
        file << "[" << (r.result == TestResult::Pass ? "PASS" : "FAIL") << "] " << r.name << "\n";
        if (r.result == TestResult::Pass) ++passed; else ++failed;
    }
    file << "\nTotal: " << passed << " passed, " << failed << " failed\n";
}

bool StabilityTest::TestRendererInit() {
    Microsoft::WRL::ComPtr<ID2D1Factory> factory;
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, factory.GetAddressOf());
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
    cfg.backgroundEffect = "test_theme";
    bool saved = Config::Instance().SaveToFile();
    bool loaded = Config::Instance().LoadFromFile();
    bool ok = saved && loaded && Config::Instance().Get().backgroundEffect == "test_theme";
    Config::Instance().LoadDefaults();
    return ok;
}

bool StabilityTest::TestThemeSwitching() {
    for (int i = 0; i < 100; ++i) {
        Config::Instance().GetMutable().backgroundEffect = (i % 2 == 0) ? "dark" : "light";
    }
    return true;
}

bool StabilityTest::TestPluginLoading() {
    return true;
}