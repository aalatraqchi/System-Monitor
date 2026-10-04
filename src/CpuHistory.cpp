#include "CpuHistory.h"

#include <cmath>

void CpuHistory::record(std::int64_t timeMs, std::optional<double> percent)
{
    if (!points_.empty() && timeMs < points_.back().timeMs) return;
    while (!points_.empty() && timeMs - points_.front().timeMs > windowMs)
        points_.pop_front();
    // Manual Refresh must not turn a minute of history into a burst of samples.
    if (!points_.empty() && timeMs - points_.back().timeMs < 1000) return;
    if (percent && (!std::isfinite(*percent) || *percent < 0.0 || *percent > 100.0))
        percent.reset();
    points_.push_back({timeMs, percent});
}
