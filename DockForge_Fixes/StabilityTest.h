#pragma once
#include <string>
#include <functional>
#include <vector>
#include <chrono>
#include <atomic>

enum class TestResult { Pass, Fail, Skip, Timeout };

struct TestCase {
    std::string name;
    std::string category; // "memory", "render", "shell", "stress", "edge"
    std::function<TestResult()> run;
    int timeoutSeconds = 30;
    bool critical = false; // If true, failure stops all tests
};

struct TestReport {
    std::string name;
    TestResult result;
    std::string message;
    float durationMs = 0.0f;
};

class StabilityTest {
public:
    static StabilityTest& Instance();

    // Register built-in tests
    void RegisterDefaultTests();
    void RegisterTest(const TestCase& test);

    // Run all or category
    std::vector<TestReport> RunAll();
    std::vector<TestReport> RunCategory(const std::string& category);
    std::vector<TestReport> RunCriticalOnly();

    // 72h simulation (accelerated)
    struct SimulationConfig {
        float realTimeHours = 72.0f;
        float timeScale = 60.0f; // 1 real second = 60 simulated seconds
        bool testMemory = true;
        bool testRender = true;
        bool testShell = true;
        bool logEveryHour = true;
    };
    bool RunLongTermSimulation(const SimulationConfig& config);

    // Stress tests
    bool StressTestIconLoading(int iterations);
    bool StressTestWindowEnumeration(int iterations);
    bool StressTestAnimationLoop(int seconds);
    bool StressTestMemoryAllocation(int mbTarget);

    // Edge case tests
    bool TestDPIPerMonitor();
    bool TestHDRDisplay();
    bool TestUWPApps();
    bool TestHighIconCount();
    bool TestRapidMonitorChange();
    bool TestFullscreenDetection();

    // Reports
    void PrintSummary(const std::vector<TestReport>& reports) const;
    void WriteReport(const std::vector<TestReport>& reports, const std::string& path) const;

private:
    StabilityTest() = default;
    std::vector<TestCase> m_tests;

    TestReport ExecuteTest(const TestCase& test);
    bool SimulateHour(float deltaSimulatedHours);
};
