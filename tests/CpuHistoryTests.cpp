#include "CpuHistory.h"
#include <cstdlib>
#include <iostream>
#include <limits>

int main()
{
    auto require = [](bool condition, const char* message) {
        if (!condition) { std::cerr << message << '\n'; std::exit(1); }
    };
    CpuHistory history;
    history.record(0, std::nullopt);
    history.record(1000, 25.0);
    history.record(1100, 90.0);
    require(history.points().size() == 2, "Rapid refresh added a history point");
    history.record(2000, std::numeric_limits<double>::quiet_NaN());
    history.record(3000, 101.0);
    require(!history.points()[2].percent && !history.points()[3].percent, "Invalid samples were plotted");
    history.record(2500, 10.0);
    require(history.points().size() == 4, "Out-of-order time accepted");
    history.record(60000, 0.0);
    require(history.points().front().timeMs == 0, "Window boundary removed early");
    history.record(61000, 100.0);
    require(history.points().front().timeMs == 1000, "Expired history retained");
    history.record(130000, 50.0);
    require(history.points().size() == 1, "Long pause retained stale data");

    CpuHistory continuous;
    for (int seconds = 0; seconds <= 180; ++seconds) continuous.record(seconds * 1000, 50.0);
    require(continuous.points().size() == 61 && continuous.points().front().timeMs == 120000,
            "History did not stay bounded to 60 seconds");
    std::cout << "History timing, gaps, and bounded retention passed\n";
}
