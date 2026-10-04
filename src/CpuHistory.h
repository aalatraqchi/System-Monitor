#pragma once

#include <cstdint>
#include <deque>
#include <optional>

struct CpuHistoryPoint {
    std::int64_t timeMs;
    std::optional<double> percent;
};

class CpuHistory {
public:
    static constexpr std::int64_t windowMs = 60000;
    // Monotonic timestamps keep the window accurate even when a timer runs late.
    void record(std::int64_t timeMs, std::optional<double> percent);
    const std::deque<CpuHistoryPoint>& points() const { return points_; }
private:
    std::deque<CpuHistoryPoint> points_;
};
