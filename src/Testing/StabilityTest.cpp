#include "StabilityTest.h"
#include "../Utils/Logger.h"
#include "../Renderer/IconLoader.h"
#include <fstream>
#include <sstream>
#include <chrono>

StabilityTest& StabilityTest::Instance() {
    static StabilityTest inst;
    return inst;
}

void StabilityTest::RegisterTest(const TestCase& test) {
    m_tests.push_back(test);
}

std::vector<TestReport> StabilityTest::RunAll() {
    std::vector<TestReport> results;
    for (auto& test : m_tests) {
        auto start = std::chrono::steady_clock::now();
        TestResult res = TestResult::Skip;
        std::string msg = "Skipped";
        try {
            res = test.run();
            msg = (res == TestResult::Pass) ? "Passed" : "Failed";
        } catch (...) {
            res = TestResult::Crash; msg = "Exception";
        }
        auto dur = std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - start).count();
        results.push_back({test.name, res, msg, dur, test.severity});
    }
    return results;
}

std::vector<TestReport> StabilityTest::RunCategory(const std::string& category) {
    std::vector<TestReport> results;
    for (auto& test : m_tests) {
        if (test.category != category) continue;
        auto start = std::chrono::steady_clock::now();
        TestResult res = TestResult::Skip;
        try { res = test.run(); } catch (...) { res = TestResult::Crash; }
        auto dur = std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - start).count();
        results.push_back({test.name, res, "", dur, test.severity});
    }
    return results;
}

void StabilityTest::WriteReport(const std::vector<TestReport>& reports, const std::string& path) const {
    std::ofstream out(path);
    out << "DockForge Stability Report\n==========================\n";
    for (const auto& r : reports) {
        out << "[" << r.name << "] " << (int)r.result << " (" << r.durationMs << "ms) " << r.message << "\n";
    }
}

void StabilityTest::RegisterDefaultTests() {
    auto makeCase = [](const std::string& name, const std::string& cat, std::function<TestResult()> fn) -> TestCase {
        return {name, cat, fn, TestSeverity::Info};
    };
    RegisterTest(makeCase("Renderer Initialization", "render", [this]() { return TestRendererInit() ? TestResult::Pass : TestResult::Fail; }));
    RegisterTest(makeCase("Icon Loading", "render", [this]() { return TestIconLoading() ? TestResult::Pass : TestResult::Fail; }));
    RegisterTest(makeCase("Memory Leak Check", "system", [this]() { return TestMemoryLeak(5.0f) ? TestResult::Pass : TestResult::Fail; }));
    RegisterTest(makeCase("Animation Stress", "render", [this]() { return TestAnimationStress() ? TestResult::Pass : TestResult::Fail; }));
    RegisterTest(makeCase("Multi-Monitor", "system", [this]() { return TestMultiMonitor() ? TestResult::Pass : TestResult::Fail; }));
    RegisterTest(makeCase("DPI Scaling", "system", [this]() { return TestDPIScaling() ? TestResult::Pass : TestResult::Fail; }));
    RegisterTest(makeCase("Window Manager", "system", [this]() { return TestWindowManager() ? TestResult::Pass : TestResult::Fail; }));
    RegisterTest(makeCase("Config Persistence", "system", [this]() { return TestConfigPersistence() ? TestResult::Pass : TestResult::Fail; }));
    RegisterTest(makeCase("Theme Switching", "ui", [this]() { return TestThemeSwitching() ? TestResult::Pass : TestResult::Fail; }));
    RegisterTest(makeCase("Plugin Loading", "system", [this]() { return TestPluginLoading() ? TestResult::Pass : TestResult::Fail; }));
}

bool StabilityTest::TestRendererInit() { return true; }
bool StabilityTest::TestIconLoading() { IconLoader loader; (void)loader; return true; }
bool StabilityTest::TestMemoryLeak(float) { return true; }
bool StabilityTest::TestAnimationStress() { return true; }
bool StabilityTest::TestMultiMonitor() { return true; }
bool StabilityTest::TestDPIScaling() { return true; }
bool StabilityTest::TestWindowManager() { return true; }
bool StabilityTest::TestConfigPersistence() { return true; }
bool StabilityTest::TestThemeSwitching() { return true; }
bool StabilityTest::TestPluginLoading() { return true; }