#include "CpuUsageTracker.h"
#include <cmath>
#include <iostream>
#include <limits>

int main()
{
    int failures = 0;
    auto expect = [&](bool condition, const char* name) {
        if (!condition) {
            std::cerr << "Failed: " << name << '\n';
            ++failures;
        }
    };
    auto equals = [](const std::optional<double>& actual, double expected) {
        return actual && std::abs(*actual - expected) < 0.000001;
    };

    CpuUsageTracker tracker;
    expect(!tracker.update({100, 200, 100}), "first reading waits for baseline");
    // 400 total ticks, 150 idle ticks: 250 / 400 = 62.5% busy.
    expect(equals(tracker.update({250, 500, 200}), 62.5), "kernel includes idle");
    expect(equals(tracker.update({250, 500, 200}), 62.5), "no elapsed ticks preserves last result");
    expect(equals(tracker.update({350, 600, 200}), 0.0), "fully idle interval");
    expect(equals(tracker.update({350, 700, 300}), 100.0), "fully busy interval");
    expect(!tracker.update({0, 0, 0}), "counter reset does not underflow");
    expect(equals(tracker.update({10, 20, 0}), 50.0), "recovers after counter reset");
    expect(!tracker.update({40, 30, 0}), "rejects idle time exceeding kernel time");
    tracker.reset();
    expect(!tracker.update({0, 0, 0}), "explicit reset discards baseline");
    const auto maximum = std::numeric_limits<std::uint64_t>::max();
    expect(equals(tracker.update({0, maximum, maximum}), 100.0), "large combined counters do not overflow");
    return failures == 0 ? 0 : 1;
}
