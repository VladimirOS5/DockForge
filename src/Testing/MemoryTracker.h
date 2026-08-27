#pragma once
#include <cstddef>
#include <string>
#include <unordered_map>
#include <mutex>
#include <vector>

struct AllocationRecord {
    size_t size = 0;
    const char* file = nullptr;
    int line = 0;
    const char* function = nullptr;
    size_t callStackHash = 0;
    bool freed = false;
};

class MemoryTracker {
public:
    static MemoryTracker& Instance();

    void Track(void* ptr, size_t size, const char* file, int line, const char* func);
    void Untrack(void* ptr);
    void MarkFreed(void* ptr);

    // Reports
    size_t GetLiveAllocationCount() const;
    size_t GetLiveBytes() const;
    std::vector<AllocationRecord> GetLeaks() const;
    void PrintReport() const;
    void WriteReportToFile(const std::string& path) const;

    // 72h stability metrics
    struct StabilityMetrics {
        size_t peakAllocations = 0;
        size_t peakBytes = 0;
        size_t totalAllocations = 0;
        size_t totalFrees = 0;
        size_t currentAllocations = 0;
        size_t currentBytes = 0;
        double avgAllocationSize = 0.0;
    };
    StabilityMetrics GetMetrics() const;
    void ResetMetrics();

    // Enable/disable tracking (performance in release)
    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }

private:
    MemoryTracker() = default;
    mutable std::mutex m_mutex;
    std::unordered_map<void*, AllocationRecord> m_allocations;
    size_t m_totalAllocations = 0;
    size_t m_totalFrees = 0;
    size_t m_peakAllocations = 0;
    size_t m_peakBytes = 0;
    bool m_enabled = true;
};

// Macros for automatic tracking
#ifdef DOCKFORGE_MEMORY_TRACKING
    #define DF_ALLOC(size) MemoryTracker::Instance().Track(::operator new(size), size, __FILE__, __LINE__, __func__); ::operator new(size)
    #define DF_FREE(ptr) do { MemoryTracker::Instance().Untrack(ptr); ::operator delete(ptr); } while(0)
    #define DF_TRACK(ptr, size) MemoryTracker::Instance().Track(ptr, size, __FILE__, __LINE__, __func__)
    #define DF_UNTRACK(ptr) MemoryTracker::Instance().Untrack(ptr)
#else
    #define DF_ALLOC(size) ::operator new(size)
    #define DF_FREE(ptr) ::operator delete(ptr)
    #define DF_TRACK(ptr, size) ((void)0)
    #define DF_UNTRACK(ptr) ((void)0)
#endif
