#include "MemoryTracker.h"
#include "Logger.h"

MemoryTracker& MemoryTracker::Instance() {
    static MemoryTracker inst;
    return inst;
}
void MemoryTracker::PrintReport() const {
    LOG_INFO("Memory report: tracking not fully implemented yet.");
}
void MemoryTracker::TrackAllocation(size_t bytes, const std::string& tag) { (void)bytes; (void)tag; }
void MemoryTracker::TrackDeallocation(size_t bytes, const std::string& tag) { (void)bytes; (void)tag; }