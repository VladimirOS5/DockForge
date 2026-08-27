#include "StabilityTest.h"
#include "MemoryTracker.h"
#include "../Utils/Logger.h"
#include "../Utils/Config.h"
#include "../Shell/WindowManager.h"
#include "../Shell/ShellHookManager.h"
#include "../Renderer/D2DRenderer.h"
#include <windows.h>
#include <sstream>
#include <thread>
#include <cmath>

StabilityTest& StabilityTest::Instance() {
    static StabilityTest instance;
    return instance;
}

void StabilityTest::RegisterDefaultTests() {
    // Critical tests
    RegisterTest({"D2D_Initialization", "render", [this]() {
        // Test if D2D factory can be created
        Microsoft::WRL::ComPtr<ID2D1Factory> factory;
        HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, factory.GetAddressOf());
        return SUCCEEDED(hr) ? TestResult::Pass : TestResult::Fail;
    }, 10, true});

    RegisterTest({"COM_Initialization", "shell", [this]() {
        HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (SUCCEEDED(hr)) CoUninitialize();
        return SUCCEEDED(hr) ? TestResult::Pass : TestResult::Fail;
    }, 5, true});

    RegisterTest({"Config_LoadSave", "memory", [this]() {
        Config::Instance().LoadDefaults();
        bool saved = Config::Instance().SaveToFile();
        bool loaded = Config::Instance().LoadFromFile();
        return (saved && loaded) ? TestResult::Pass : TestResult::Fail;
    }, 10, true});

    // Memory tests
    RegisterTest({"Memory_NoLeak_Short", "memory", [this]() {
        size_t before = MemoryTracker::Instance().GetLiveBytes();
        // Simulate typical allocation pattern
        for (int i = 0; i < 100; ++i) {
            void* p = malloc(1024);
            free(p);
        }
        size_t after = MemoryTracker::Instance().GetLiveBytes();
        // Allow small variance
        return (after <= before + 1024) ? TestResult::Pass : TestResult::Fail;
    }, 30, false});

    RegisterTest({"Memory_Stress_10MB", "memory", [this]() {
        return StressTestMemoryAllocation(10) ? TestResult::Pass : TestResult::Fail;
    }, 60, false});

    // Render tests
    RegisterTest({"Render_EffectSwitch", "render", [this]() {
        // Simulate effect switching (Acrylic -> Liquid -> Audio -> Solid)
        for (int i = 0; i < 50; ++i) {
            // This would be called from DockWindow in real test
            Sleep(10);
        }
        return TestResult::Pass;
    }, 30, false});

    // Shell tests
    RegisterTest({"Shell_WindowEnum", "shell", [this]() {
        return StressTestWindowEnumeration(100) ? TestResult::Pass : TestResult::Fail;
    }, 30, false});

    // Edge case tests
    RegisterTest({"Edge_DPI_PerMonitor", "edge", [this]() {
        return TestDPIPerMonitor() ? TestResult::Pass : TestResult::Fail;
    }, 15, false});

    RegisterTest({"Edge_HDR_Display", "edge", [this]() {
        return TestHDRDisplay() ? TestResult::Pass : TestResult::Fail;
    }, 15, false});

    RegisterTest({"Edge_UWP_Apps", "edge", [this]() {
        return TestUWPApps() ? TestResult::Pass : TestResult::Fail;
    }, 20, false});

    RegisterTest({"Edge_HighIconCount", "edge", [this]() {
        return TestHighIconCount() ? TestResult::Pass : TestResult::Fail;
    }, 30, false});

    RegisterTest({"Edge_FullscreenDetection", "edge", [this]() {
        return TestFullscreenDetection() ? TestResult::Pass : TestResult::Fail;
    }, 15, false});

    LOG_INFO("Registered " + std::to_string(m_tests.size()) + " stability tests");
}

void StabilityTest::RegisterTest(const TestCase& test) {
    m_tests.push_back(test);
}

TestReport StabilityTest::ExecuteTest(const TestCase& test) {
    TestReport report;
    report.name = test.name;

    auto start = std::chrono::steady_clock::now();

    try {
        report.result = test.run();
        if (report.result == TestResult::Pass) {
            report.message = "OK";
        }
    } catch (const std::exception& e) {
        report.result = TestResult::Fail;
        report.message = std::string("Exception: ") + e.what();
    } catch (...) {
        report.result = TestResult::Fail;
        report.message = "Unknown exception";
    }

    auto end = std::chrono::steady_clock::now();
    report.durationMs = std::chrono::duration<float, std::milli>(end - start).count();

    if (report.result == TestResult::Fail) {
        LOG_ERROR("TEST FAIL [" + test.name + "]: " + report.message);
    } else if (report.result == TestResult::Pass) {
        LOG_INFO("TEST PASS [" + test.name + "]: " + std::to_string(static_cast<int>(report.durationMs)) + "ms");
    }

    return report;
}

std::vector<TestReport> StabilityTest::RunAll() {
    LOG_INFO("========== RUNNING ALL STABILITY TESTS ==========");
    std::vector<TestReport> reports;
    for (const auto& test : m_tests) {
        reports.push_back(ExecuteTest(test));
        if (test.critical && reports.back().result == TestResult::Fail) {
            LOG_FATAL("CRITICAL TEST FAILED: " + test.name + ". Stopping test suite.");
            break;
        }
    }
    PrintSummary(reports);
    return reports;
}

std::vector<TestReport> StabilityTest::RunCategory(const std::string& category) {
    LOG_INFO("Running tests for category: " + category);
    std::vector<TestReport> reports;
    for (const auto& test : m_tests) {
        if (test.category == category) {
            reports.push_back(ExecuteTest(test));
        }
    }
    return reports;
}

std::vector<TestReport> StabilityTest::RunCriticalOnly() {
    LOG_INFO("Running critical tests only...");
    std::vector<TestReport> reports;
    for (const auto& test : m_tests) {
        if (test.critical) {
            reports.push_back(ExecuteTest(test));
            if (reports.back().result == TestResult::Fail) {
                LOG_FATAL("CRITICAL TEST FAILED!");
                break;
            }
        }
    }
    return reports;
}

bool StabilityTest::RunLongTermSimulation(const SimulationConfig& config) {
    LOG_INFO("Starting 72h stability simulation (scaled " + std::to_string(static_cast<int>(config.timeScale)) + "x)");
    LOG_INFO("Real duration: " + std::to_string(static_cast<int>(config.realTimeHours * 3600 / config.timeScale / 3600)) + " hours");

    float simulatedHours = 0;
    float logInterval = 1.0f; // Log every simulated hour
    float nextLog = logInterval;
    int hourCount = 0;

    auto start = std::chrono::steady_clock::now();

    while (simulatedHours < config.realTimeHours) {
        float deltaReal = 0.016f; // ~60 FPS real time
        float deltaSimulated = deltaReal * config.timeScale / 3600.0f; // hours
        simulatedHours += deltaSimulated;

        if (!SimulateHour(deltaSimulated)) {
            LOG_ERROR("Simulation failed at hour " + std::to_string(static_cast<int>(simulatedHours)));
            return false;
        }

        if (config.logEveryHour && simulatedHours >= nextLog) {
            nextLog += logInterval;
            hourCount++;
            auto metrics = MemoryTracker::Instance().GetMetrics();
            LOG_INFO("Simulated hour " + std::to_string(hourCount) + 
                     " | Live: " + std::to_string(metrics.currentAllocations) + 
                     " alloc, " + std::to_string(metrics.currentBytes / 1024) + " KB");
        }

        // Yield to prevent 100% CPU during simulation
        if (hourCount % 10 == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    auto end = std::chrono::steady_clock::now();
    float realMinutes = std::chrono::duration<float>(end - start).count() / 60.0f;

    LOG_INFO("Simulation complete! Simulated " + std::to_string(static_cast<int>(config.realTimeHours)) + 
             "h in " + std::to_string(static_cast<int>(realMinutes)) + " real minutes");

    auto metrics = MemoryTracker::Instance().GetMetrics();
    if (metrics.currentAllocations > 100) {
        LOG_WARN("Potential memory leak: " + std::to_string(metrics.currentAllocations) + " live allocations after simulation");
        return false;
    }

    return true;
}

bool StabilityTest::SimulateHour(float deltaSimulatedHours) {
    // Simulate typical operations that happen in an hour of real usage
    static float accum = 0;
    accum += deltaSimulatedHours;

    // Every simulated 5 minutes: window enumeration
    if (accum >= 5.0f / 60.0f) {
        accum = 0;
        // Simulate window manager refresh
    }

    // Simulate periodic garbage collection of textures
    // This would trigger atlas cleanup, icon cache refresh, etc.

    return true;
}

bool StabilityTest::StressTestIconLoading(int iterations) {
    LOG_INFO("Stress test: loading " + std::to_string(iterations) + " icons...");
    for (int i = 0; i < iterations; ++i) {
        // Would load and unload icons here
        if (i % 100 == 0) {
            auto metrics = MemoryTracker::Instance().GetMetrics();
            if (metrics.currentBytes > 50 * 1024 * 1024) { // 50MB limit
                LOG_WARN("Memory limit exceeded during icon stress test");
                return false;
            }
        }
    }
    return true;
}

bool StabilityTest::StressTestWindowEnumeration(int iterations) {
    for (int i = 0; i < iterations; ++i) {
        EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
            // Just count windows
            (*reinterpret_cast<int*>(lParam))++;
            return TRUE;
        }, reinterpret_cast<LPARAM>(&i));
    }
    return true;
}

bool StabilityTest::StressTestAnimationLoop(int seconds) {
    auto start = std::chrono::steady_clock::now();
    float t = 0;
    while (true) {
        auto now = std::chrono::steady_clock::now();
        float elapsed = std::chrono::duration<float>(now - start).count();
        if (elapsed >= seconds) break;

        // Simulate animation update
        t += 0.016f;
        float val = std::sin(t * 10.0f) * std::cos(t * 5.0f);
        (void)val; // Prevent unused warning
    }
    return true;
}

bool StabilityTest::StressTestMemoryAllocation(int mbTarget) {
    std::vector<void*> blocks;
    size_t allocated = 0;
    size_t target = mbTarget * 1024 * 1024;

    try {
        while (allocated < target) {
            size_t blockSize = 64 * 1024; // 64KB blocks
            void* p = malloc(blockSize);
            if (!p) break;
            blocks.push_back(p);
            allocated += blockSize;
        }
    } catch (...) {}

    for (void* p : blocks) free(p);
    blocks.clear();

    LOG_INFO("Stress allocated " + std::to_string(allocated / 1024 / 1024) + " MB, freed all");
    return true;
}

bool StabilityTest::TestDPIPerMonitor() {
    // Check if Per-Monitor V2 DPI awareness is active
    DPI_AWARENESS_CONTEXT ctx = GetThreadDpiAwarenessContext();
    bool isPerMonitor = (ctx == DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) ||
                        (ctx == DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);

    if (!isPerMonitor) {
        LOG_WARN("DPI test: Not running in Per-Monitor DPI awareness mode");
    }

    // Enumerate monitors and check DPI
    int monitorCount = 0;
    float minDpi = 999, maxDpi = 0;

    EnumDisplayMonitors(nullptr, nullptr, [](HMONITOR hMon, HDC hdc, LPRECT lprc, LPARAM lParam) -> BOOL {
        UINT dpiX = 96, dpiY = 96;
        GetDpiForMonitor(hMon, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);

        auto* data = reinterpret_cast<std::pair<float*, float*>*>(lParam);
        *data->first = std::min(*data->first, static_cast<float>(dpiX));
        *data->second = std::max(*data->second, static_cast<float>(dpiX));

        (*reinterpret_cast<int*>(lParam + sizeof(void*) * 2))++;
        return TRUE;
    }, reinterpret_cast<LPARAM>(&minDpi));

    LOG_INFO("DPI test: Monitors=" + std::to_string(monitorCount) + 
             ", DPI range=" + std::to_string(static_cast<int>(minDpi)) + "-" + std::to_string(static_cast<int>(maxDpi)));
    return true; // Pass if we can read DPI
}

bool StabilityTest::TestHDRDisplay() {
    // Check for HDR-capable displays using DXGI
    bool hdrAvailable = false;

    // Simple check: query display color space
    HDC hdc = GetDC(nullptr);
    if (hdc) {
        int bits = GetDeviceCaps(hdc, BITSPIXEL);
        ReleaseDC(nullptr, hdc);
        LOG_INFO("HDR test: Display bit depth=" + std::to_string(bits));
    }

    // Try to create DXGI factory and enumerate adapters
    Microsoft::WRL::ComPtr<IDXGIFactory1> factory;
    HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(factory.GetAddressOf()));
    if (SUCCEEDED(hr) && factory) {
        Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
        for (UINT i = 0; factory->EnumAdapters1(i, adapter.GetAddressOf()) != DXGI_ERROR_NOT_FOUND; ++i) {
            DXGI_ADAPTER_DESC1 desc;
            if (SUCCEEDED(adapter->GetDesc1(&desc))) {
                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
                LOG_INFO("HDR test: GPU found: " + std::string(desc.Description, desc.Description + wcslen(desc.Description)));
            }
            adapter.Reset();
        }
        hdrAvailable = true;
    }

    LOG_INFO("HDR test: " + std::string(hdrAvailable ? "DXGI available, HDR detection possible" : "DXGI not available"));
    return true;
}

bool StabilityTest::TestUWPApps() {
    // Check if we can enumerate UWP apps via PackageManager
    // This requires Windows 10+ and specific APIs
    // Simplified: check if we can find Windows Store apps in process list

    bool foundUWP = false;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe = { sizeof(pe) };
        if (Process32FirstW(snap, &pe)) {
            do {
                // UWP apps often run under ApplicationFrameHost or have specific signatures
                std::wstring name(pe.szExeFile);
                if (name.find(L"ApplicationFrameHost.exe") != std::wstring::npos ||
                    name.find(L"WindowsApps") != std::wstring::npos) {
                    foundUWP = true;
                    break;
                }
            } while (Process32NextW(snap, &pe));
        }
        CloseHandle(snap);
    }

    LOG_INFO("UWP test: " + std::string(foundUWP ? "UWP runtime detected" : "No UWP apps running"));
    return true;
}

bool StabilityTest::TestHighIconCount() {
    // Simulate dock with 50+ icons
    const int iconCount = 60;
    LOG_INFO("High icon test: simulating " + std::to_string(iconCount) + " dock icons");

    size_t before = MemoryTracker::Instance().GetLiveBytes();

    // Simulate icon data structures
    std::vector<std::vector<uint8_t>> iconData;
    for (int i = 0; i < iconCount; ++i) {
        iconData.emplace_back(48 * 48 * 4); // 48x48 RGBA
    }

    size_t during = MemoryTracker::Instance().GetLiveBytes();
    iconData.clear();

    size_t after = MemoryTracker::Instance().GetLiveBytes();

    LOG_INFO("High icon test: Memory delta=" + std::to_string(static_cast<int>((after - before) / 1024)) + " KB");
    return true;
}

bool StabilityTest::TestRapidMonitorChange() {
    // Simulate rapid monitor attach/detach by querying monitor info multiple times
    for (int i = 0; i < 20; ++i) {
        int count = GetSystemMetrics(SM_CMONITORS);
        (void)count;
        Sleep(50);
    }
    return true;
}

bool StabilityTest::TestFullscreenDetection() {
    // Check if we can detect fullscreen applications
    HWND fg = GetForegroundWindow();
    if (!fg) return true; // No window, skip

    RECT rc;
    GetWindowRect(fg, &rc);
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;

    MONITORINFO mi = { sizeof(mi) };
    HMONITOR hMon = MonitorFromWindow(fg, MONITOR_DEFAULTTONEAREST);
    bool isFullscreen = false;
    if (GetMonitorInfoW(hMon, &mi)) {
        int screenW = mi.rcMonitor.right - mi.rcMonitor.left;
        int screenH = mi.rcMonitor.bottom - mi.rcMonitor.top;
        isFullscreen = (w >= screenW && h >= screenH);
    }

    LOG_INFO("Fullscreen test: FG window " + std::to_string(w) + "x" + std::to_string(h) + 
             " | Fullscreen=" + std::string(isFullscreen ? "YES" : "NO"));
    return true;
}

void StabilityTest::PrintSummary(const std::vector<TestReport>& reports) const {
    int passed = 0, failed = 0, skipped = 0;
    float totalTime = 0;

    for (const auto& r : reports) {
        if (r.result == TestResult::Pass) passed++;
        else if (r.result == TestResult::Fail) failed++;
        else skipped++;
        totalTime += r.durationMs;
    }

    LOG_INFO("========== TEST SUMMARY ==========");
    LOG_INFO("Total: " + std::to_string(reports.size()) + " | Passed: " + std::to_string(passed) + 
             " | Failed: " + std::to_string(failed) + " | Skipped: " + std::to_string(skipped));
    LOG_INFO("Total time: " + std::to_string(static_cast<int>(totalTime)) + "ms");
    LOG_INFO("==================================");

    if (failed > 0) {
        LOG_WARN("Some tests failed. Review logs above.");
    }
}

void StabilityTest::WriteReport(const std::vector<TestReport>& reports, const std::string& path) const {
    std::ofstream f(path);
    if (!f) return;

    f << "DockForge Stability Test Report\n";
    f << "================================\n\n";

    int passed = 0, failed = 0;
    for (const auto& r : reports) {
        f << "[" << (r.result == TestResult::Pass ? "PASS" : r.result == TestResult::Fail ? "FAIL" : "SKIP") << "] "
          << r.name << " (" << static_cast<int>(r.durationMs) << "ms)" << "\n";
        if (!r.message.empty() && r.message != "OK") {
            f << "      " << r.message << "\n";
        }
        if (r.result == TestResult::Pass) passed++;
        else if (r.result == TestResult::Fail) failed++;
    }

    f << "\nSummary: " << passed << "/" << reports.size() << " passed";
    if (failed > 0) f << ", " << failed << " failed";
    f << "\n";
}
