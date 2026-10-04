#pragma once

#include <optional>

// Missing values mean a measurement is not available yet, rather than zero usage.
struct SystemSnapshot {
    std::optional<double> cpuPercent;
    std::optional<double> ramPercent;
};
