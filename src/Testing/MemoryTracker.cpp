#include "MemoryTracker.h"
#include "../Utils/Logger.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>

MemoryTracker& MemoryTracker::Instance() {
    static MemoryTracker instance;
    return instance;
}

void MemoryTracker::Track(void* ptr, size_t size, const char* file, int line, const char* func) {
    if (!m_enabled || !ptr) return;
    std::lock_guard<std::mutex> lock(m_mutex);

    AllocationRecord rec;
    rec.size = size;
    rec.file = file;
    rec.line = line;
    rec.function = func;
    rec.freed = false;

    // Simple hash of call site
    rec.callStackHash = std::hash<std::string>{}(std::string(file) + ":" + std::to_string(line));

    m_allocations[ptr] = rec;
    m_totalAllocations++;

    size_t liveCount = 0;
    size_t liveBytes = 0;
    for (const auto& [p, r] : m_allocations) {
        if (!r.freed) { liveCount++; liveBytes += r.size; }
    }

    if (liveCount > m_peakAllocations) m_peakAllocations = liveCount;
    if (liveBytes > m_peakBytes) m_peakBytes = liveBytes;
}

void MemoryTracker::Untrack(void* ptr) {
    if (!m_enabled || !ptr) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_allocations.find(ptr);
    if (it != m_allocations.end()) {
        it->second.freed = true;
        m_totalFrees++;
    }
}

void MemoryTracker::MarkFreed(void* ptr) {
    Untrack(ptr);
}

size_t MemoryTracker::GetLiveAllocationCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t count = 0;
    for (const auto& [p, r] : m_allocations) {
        if (!r.freed) count++;
    }
    return count;
}

size_t MemoryTracker::GetLiveBytes() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t bytes = 0;
    for (const auto& [p, r] : m_allocations) {
        if (!r.freed) bytes += r.size;
    }
    return bytes;
}

std::vector<AllocationRecord> MemoryTracker::GetLeaks() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<AllocationRecord> leaks;
    for (const auto& [p, r] : m_allocations) {
        if (!r.freed) leaks.push_back(r);
    }
    return leaks;
}

void MemoryTracker::PrintReport() const {
    auto metrics = GetMetrics();
    auto leaks = GetLeaks();

    LOG_INFO("========== MEMORY REPORT ==========");
    LOG_INFO("Live allocations: " + std::to_string(metrics.currentAllocations));
    LOG_INFO("Live bytes: " + std::to_string(metrics.currentBytes) + " bytes (" + 
             std::to_string(metrics.currentBytes / 1024) + " KB)");
    LOG_INFO("Peak allocations: " + std::to_string(metrics.peakAllocations));
    LOG_INFO("Peak bytes: " + std::to_string(metrics.peakBytes) + " bytes (" + 
             std::to_string(metrics.peakBytes / 1024) + " KB)");
    LOG_INFO("Total allocations: " + std::to_string(metrics.totalAllocations));
    LOG_INFO("Total frees: " + std::to_string(metrics.totalFrees));
    LOG_INFO("Avg allocation size: " + std::to_string(static_cast<int>(metrics.avgAllocationSize)) + " bytes");

    if (!leaks.empty()) {
        LOG_WARN("MEMORY LEAKS DETECTED: " + std::to_string(leaks.size()) + " allocations");
        for (const auto& leak : leaks) {
            std::string loc = (leak.file ? leak.file : "?") + std::string(":") + std::to_string(leak.line);
            LOG_WARN("  Leak: " + std::to_string(leak.size) + " bytes at " + loc + 
                     " [" + (leak.function ? leak.function : "?") + "]");
        }
    } else {
        LOG_INFO("No memory leaks detected!");
    }
    LOG_INFO("===================================");
}

void MemoryTracker::WriteReportToFile(const std::string& path) const {
    std::ofstream f(path);
    if (!f) return;

    auto metrics = GetMetrics();
    auto leaks = GetLeaks();

    f << "DockForge Memory Report\n";
    f << "=======================\n";
    f << "Live allocations: " << metrics.currentAllocations << "\n";
    f << "Live bytes: " << metrics.currentBytes << " (" << (metrics.currentBytes / 1024) << " KB)\n";
    f << "Peak allocations: " << metrics.peakAllocations << "\n";
    f << "Peak bytes: " << metrics.peakBytes << " (" << (metrics.peakBytes / 1024) << " KB)\n";
    f << "Total allocations: " << metrics.totalAllocations << "\n";
    f << "Total frees: " << metrics.totalFrees << "\n\n";

    if (!leaks.empty()) {
        f << "LEAKS (" << leaks.size() << "):\n";
        for (const auto& leak : leaks) {
            f << "  " << leak.size << " bytes at " 
              << (leak.file ? leak.file : "?") << ":" << leak.line
              << " [" << (leak.function ? leak.function : "?") << "]\n";
        }
    } else {
        f << "No memory leaks detected.\n";
    }
}

MemoryTracker::StabilityMetrics MemoryTracker::GetMetrics() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    StabilityMetrics m;
    m.peakAllocations = m_peakAllocations;
    m.peakBytes = m_peakBytes;
    m.totalAllocations = m_totalAllocations;
    m.totalFrees = m_totalFrees;
    m.currentAllocations = 0;
    m.currentBytes = 0;

    double totalSize = 0;
    for (const auto& [p, r] : m_allocations) {
        if (!r.freed) {
            m.currentAllocations++;
            m.currentBytes += r.size;
            totalSize += r.size;
        }
    }
    if (m.currentAllocations > 0) {
        m.avgAllocationSize = totalSize / m.currentAllocations;
    }
    return m;
}

void MemoryTracker::ResetMetrics() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_peakAllocations = 0;
    m_peakBytes = 0;
    m_totalAllocations = 0;
    m_totalFrees = 0;
    m_allocations.clear();
}
