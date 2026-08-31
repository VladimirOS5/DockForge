#pragma once
#include <string>
#include <vector>
#include <functional>

enum class TestResult { Pass, Fail, Skip, Timeout, Crash };
enum class TestSeverity { Info, Warning, Critical };

struct TestReport {
    std::string name;
    TestResult result;
    std::string message;
    float durationMs = 0.0f;
    TestSeverity severity = TestSeverity::Info;
};

struct TestCase {
    std::string name;
    std::string category;
    std::function<TestResult()> run;
    TestSeverity severity = TestSeverity::Info;
};

class StabilityTest {
public:
    static StabilityTest& Instance();
    void RegisterTest(const TestCase& test);
    std::vector<TestReport> RunAll();
    std::vector<TestReport> RunCategory(const std::string& category);
    void WriteReport(const std::vector<TestReport>& reports, const std::string& path) const;
    void RegisterDefaultTests();
    void SetTimeout(float seconds) { m_timeout = seconds; }
private:
    std::vector<TestCase> m_tests;
    float m_timeout = 30.0f;
    bool TestRendererInit();
    bool TestIconLoading();
    bool TestMemoryLeak(float durationSec);
    bool TestAnimationStress();
    bool TestMultiMonitor();
    bool TestDPIScaling();
    bool TestWindowManager();
    bool TestConfigPersistence();
    bool TestThemeSwitching();
    bool TestPluginLoading();
};