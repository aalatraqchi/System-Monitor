#include "SystemMonitor.h"

// Keep Windows headers inside the monitoring implementation, away from the UI.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

SystemSnapshot SystemMonitor::sample() const
{
    SystemSnapshot snapshot;
    MEMORYSTATUSEX status{};
    status.dwLength = sizeof(status);

    // Return an unavailable measurement on failure; the next refresh can retry.
    if (!GlobalMemoryStatusEx(&status) || status.ullTotalPhys == 0
        || status.ullAvailPhys > status.ullTotalPhys) {
        return snapshot;
    }

    // Available memory includes reusable standby memory, not just free pages.
    const auto usedBytes = status.ullTotalPhys - status.ullAvailPhys;
    snapshot.memory = MemoryUsage{
        usedBytes,
        status.ullTotalPhys,
        100.0 * static_cast<double>(usedBytes) / static_cast<double>(status.ullTotalPhys)
    };
    return snapshot;
}
