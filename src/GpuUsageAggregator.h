#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

struct GpuKey {
    std::uint64_t adapterId;
    unsigned physicalIndex;
    bool operator<(const GpuKey& other) const;
};

struct GpuEngineSample {
    std::wstring instance;
    double percent;
};

struct GpuUsage {
    double percent;
    std::wstring busiestEngine;
};

// Sum processes within each engine, then choose the busiest engine per GPU.
std::map<GpuKey, GpuUsage> aggregateGpuUsage(const std::vector<GpuEngineSample>& samples);
