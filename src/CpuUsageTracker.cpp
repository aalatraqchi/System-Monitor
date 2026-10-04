#include "CpuUsageTracker.h"

std::optional<double> CpuUsageTracker::update(const CpuTimes& current)
{
    if (!previous_) {
        previous_ = current;
        return std::nullopt; // A single cumulative reading cannot show usage.
    }

    const auto previous = *previous_;
    if (current.idle < previous.idle || current.kernel < previous.kernel
        || current.user < previous.user) {
        reset();
        previous_ = current; // Recover from counter resets without unsigned underflow.
        return std::nullopt;
    }

    const auto idle = current.idle - previous.idle;
    const auto kernel = current.kernel - previous.kernel;
    const auto user = current.user - previous.user;
    // Convert before adding so a large integer sum cannot overflow.
    const auto total = static_cast<double>(kernel) + static_cast<double>(user);
    if (total == 0.0) {
        return lastPercent_; // Rapid Refresh clicks can precede the next counter tick.
    }
    previous_ = current;
    if (idle > kernel) {
        lastPercent_.reset();
        return std::nullopt;
    }

    // Subtract idle only once: Windows already includes it in kernel time.
    const auto busy = static_cast<double>(kernel - idle) + static_cast<double>(user);
    lastPercent_ = 100.0 * busy / total;
    return lastPercent_;
}

void CpuUsageTracker::reset()
{
    previous_.reset();
    lastPercent_.reset();
}
