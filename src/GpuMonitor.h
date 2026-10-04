#pragma once

#include "GpuUsageAggregator.h"
#include <memory>

enum class GpuReadingStatus { Waiting, Live, Unavailable };
struct GpuReading {
    GpuKey key;
    std::wstring name;
    std::optional<double> percent;
    std::wstring busiestEngine;
    GpuReadingStatus status = GpuReadingStatus::Unavailable;
};
struct GpuSnapshot {
    std::vector<GpuReading> adapters;
    bool enumerationAvailable = true;
};

// Keep Windows handles private; create, sample, and destroy on the same worker.
class GpuMonitor {
public:
    GpuMonitor();
    ~GpuMonitor();
    GpuSnapshot sample();
private:
    struct State;
    std::unique_ptr<State> state_;
};
