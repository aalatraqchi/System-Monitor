#pragma once

#include <cstdint>
#include <optional>

struct MemoryUsage {
    std::uint64_t usedBytes;
    std::uint64_t totalBytes;
    double percent;
};

// Missing values mean a measurement is unavailable, rather than zero usage.
struct SystemSnapshot {
    std::optional<double> cpuPercent;
    std::optional<MemoryUsage> memory;
};
