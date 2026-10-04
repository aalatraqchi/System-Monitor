#pragma once

#include <cstdint>
#include <optional>

// Cumulative CPU time counters. Kernel time includes idle time on Windows.
struct CpuTimes {
    std::uint64_t idle;
    std::uint64_t kernel;
    std::uint64_t user;
};

// Converts consecutive readings to busy time as a percentage of total time.
// It has no Windows or Qt dependencies, making the calculation easy to test.
class CpuUsageTracker {
public:
    std::optional<double> update(const CpuTimes& current);
    void reset();

private:
    std::optional<CpuTimes> previous_;
    std::optional<double> lastPercent_;
};
