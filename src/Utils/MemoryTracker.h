#pragma once
#include <string>

class MemoryTracker {
public:
    static MemoryTracker& Instance();
    void PrintReport() const;
    void TrackAllocation(size_t bytes, const std::string& tag);
    void TrackDeallocation(size_t bytes, const std::string& tag);
private:
    MemoryTracker() = default;
};