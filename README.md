# System Monitor

A Windows desktop application using C++17 and Qt Widgets.

## Current milestone
CPU and RAM show live usage percentages; RAM also displays used/total GiB, refreshed
once per second. Refresh also samples immediately. CPU waits for its second reading before displaying a value. A sortable process table shows names, PIDs, and working-set memory in MiB; unavailable memory stays visible and sorts last.

## Build and run
Open `CMakeLists.txt` in Qt Creator and select **Desktop Qt 6.12.0 MinGW 64-bit**.
Build and run using Qt Creator, or run this from PowerShell:

```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1
.\build\SystemMonitor.exe
```

The script uses the Qt installation at `C:\Qt`. Pass `-QtRoot` if it moves.
It builds a release executable and copies the required Qt runtime dependencies.

## Code map
- `main.cpp`: starts Qt and opens the window.
- `MainWindow`: builds the UI and displays measurements.
- `SystemSnapshot`: stores available measurements.
- `SystemMonitor`: reads Windows CPU times and physical memory independently of the UI.
- CpuUsageTracker: calculates CPU usage from consecutive counters.
- ProcessMonitor: collects process snapshots and closes Windows handles automatically.
- ProcessTableModel / ProcessSortModel: display rows and sort names, PIDs, and raw memory values.

Qt owns child widgets through their parents. Layouts handle window resizing.

## Next steps
1. Add a sortable process table.
2. Add CPU history, background process collection, and UI polish.

## Version control
See [DEVELOPMENT.md](DEVELOPMENT.md) for the branch, commit, and pull-request workflow.

## CPU measurement
CPU usage is busy CPU time divided by total CPU time over the sampling interval.
Windows kernel time includes idle time, which is subtracted once. Rapid refreshes
without new counter ticks retain the last valid percentage. Failed reads or reset
counters start a fresh baseline. RAM collection remains independent.

This uses GetSystemTimes; on systems with more than 64 logical processors it covers
the calling thread's primary processor group. Its time-based percentage may differ
from Task Manager's frequency-adjusted processor utilization.

## Process memory
Memory is the working set (resident physical memory), which may include shared pages.
It can differ from Task Manager's default memory column. Protected or exited processes
show Unavailable rather than zero. Snapshots refresh every second; sorting, selection,
and scroll position are retained. Process collection runs on the GUI thread until step 7.
