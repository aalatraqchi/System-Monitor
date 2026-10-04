#include "SystemMonitor.h"

// Keep Windows headers inside the monitoring implementation, away from the UI.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace {
std::uint64_t toTicks(const FILETIME& time)
{
    return (static_cast<std::uint64_t>(time.dwHighDateTime) << 32)
        | static_cast<std::uint64_t>(time.dwLowDateTime);
}
}

SystemSnapshot SystemMonitor::sample()
{
    SystemSnapshot snapshot;
    FILETIME idle{}, kernel{}, user{};
    if (GetSystemTimes(&idle, &kernel, &user)) {
        snapshot.cpuPercent = cpuTracker_.update({toTicks(idle), toTicks(kernel), toTicks(user)});
    } else {
        // Start fresh after a failed API call instead of displaying stale CPU data.
        cpuTracker_.reset();
    }

    MEMORYSTATUSEX status{};
    status.dwLength = sizeof(status);
    // RAM and CPU are independent: a failure in one must not hide the other.
    if (GlobalMemoryStatusEx(&status) && status.ullTotalPhys > 0
        && status.ullAvailPhys <= status.ullTotalPhys) {
        // Available memory includes reusable standby memory, not just free pages.
        const auto usedBytes = status.ullTotalPhys - status.ullAvailPhys;
        snapshot.memory = MemoryUsage{
            usedBytes,
            status.ullTotalPhys,
            100.0 * static_cast<double>(usedBytes) / static_cast<double>(status.ullTotalPhys)
        };
    }
    return snapshot;
}
