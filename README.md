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
- MonitoringService: collects GPU and process data on a worker and delivers copied snapshots on the GUI thread.
- GpuMonitor / GpuUsageAggregator / GpuDashboard: read GPU counters, combine engine usage, and display per-GPU cards.
- TemperatureMonitor / TemperatureSnapshot: manage the helper, validate readings, and match sensors to cards.
- helpers/TemperatureHelper: a small C# bridge to LibreHardwareMonitor, with CPU/GPU sensor selection.
- CpuHistory / CpuHistoryGraph: retain timestamped CPU history and draw the graph.
- ProcessTableModel / ProcessSortModel: display rows and sort names, PIDs, and raw memory values.

Qt owns child widgets through their parents. Layouts handle window resizing.

## CPU history
The dashboard draws a live graph with a fixed 0–100% scale and a rolling 60-second
window. Monotonic timestamps keep delayed timer readings in the right position.
Rapid Refresh clicks do not add extra history points. Missing/invalid readings and
long pauses create gaps; old readings expire automatically. Qt Widgets/QPainter
draw the graph, so no extra chart package is needed.

## Next steps
1. Polish and package (step 10).

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
and scroll position are retained. Process collection runs on a dedicated worker thread.

## Background collection
A MonitoringWorker QObject lives on a QThread. MonitoringService stays on the GUI
thread and uses queued signals to request scans and receive copied snapshots.
MainWindow alone updates the table, selection, scroll position, and labels.

Startup, the one-second timer, and Refresh request a scan. If one is already pending
or running, additional requests are ignored; the next timer tick retries. Failed
scans retain the last table. CPU/RAM reads and graph updates remain on the GUI thread.

On exit the service requests interruption, quits the worker event loop, and waits
for the current scan to finish. The collector checks cancellation between process
reads; Windows API calls already in progress finish normally. Handles close through
RAII and Qt deletes the worker when the thread finishes. Threads are never forcefully
terminated.

The background-process-lifecycle test checks a slow scan while the GUI timer keeps
ticking, result delivery on the main thread, request coalescing, failed-scan recovery,
active-scan cancellation, repeated stop calls, and live Windows collection.

## GPU usage
Hardware adapters are identified through DXGI and matched to Windows GPU Engine
performance counters by their adapter LUID and physical-node index. Software adapters
are excluded. Process contributions are summed within each engine, then the busiest
engine is displayed for each GPU. The percentage is capped at 100%; engine percentages
are not added across independent engines. Cards show the hardware name and busiest
engine, so video encode/compute activity remains visible alongside 3D activity.

The worker owns the persistent PDH query and uses English counter paths for Windows
language independence. The first read establishes a baseline; manual refreshes less
than 900 ms apart reuse the last GPU sample. Missing/invalid counters show unavailable
usage, and failed queries retry. Valid idle counters show 0%. Known GPU names remain
visible if adapter enumeration temporarily fails. The dashboard scrolls for multiple
adapters or smaller windows. No vendor SDK or extra runtime installation is required.

Local validation requires live counters on this PC; CI permits unavailable GPU
hardware and still verifies aggregation, multiple adapters/nodes, invalid readings,
and queued GPU snapshot delivery.

References:
- https://devblogs.microsoft.com/directx/gpus-in-the-task-manager/
- https://learn.microsoft.com/en-us/windows/win32/api/pdh/nf-pdh-pdhgetformattedcounterarrayw

## Temperatures
CPU and GPU cards show Celsius readings from a small, separate LibreHardwareMonitor
0.9.6 helper. All temperature collection happens on the worker; the C++/Qt program
remains usable if the helper is missing, crashes, or times out. It uses a persistent
child process with one JSON reply per request, a 2.5-second timeout, cancellation,
and a 30-second retry delay after failure. Closing the app also closes its helper.

Build it once with a .NET 8 SDK (the .NET runtime is included in its output):

```powershell
powershell -ExecutionPolicy Bypass -File .\build-temperatures.ps1
.\build\SystemMonitor.exe
```

The script uses `build-tools\dotnet\dotnet.exe` if present, otherwise your installed
SDK. NuGet versions are recorded in `helpers/TemperatureHelper/packages.lock.json`.
For a Qt Creator build in another directory, pass `-OutputDirectory` pointing to
`temperature-helper` beside that build's SystemMonitor.exe. CMake can build the
C++ app without .NET; missing helpers show Unavailable with setup details on hover.

Intel CPU Package or AMD Tctl/Tdie is preferred; the hottest available core is an
explicitly labeled fallback. On multiple CPU packages, the hottest reading is shown.
GPU Core is used, excluding hotspot/memory temperatures. GPU names are matched only
when unique in both snapshots; duplicate model names and linked nodes show unavailable
temperature instead of assigning a reading to the wrong adapter. Usage still works.
Null, nonnumeric, and out-of-range readings show Unavailable rather than 0 °C.

Some CPU sensors require the optional PawnIO driver and administrator access. The
helper never installs a driver or requests elevation. If a supported driver is already
installed, launching the app as administrator may allow those readings. This PC's
CPU package and GPU temperatures were verified live after installing the official signed
PawnIO 2.2.0 driver and restarting System Monitor as administrator. For CPU readings on
this PC, right-click build\SystemMonitor.exe and choose Run as administrator; Windows
may show a UAC prompt. Ordinary launches still show GPU temperature, while CPU temperature
is unavailable without elevated access. Hover unavailable labels for the reason. ACPI thermal zones are
not used as a substitute for CPU package temperature.

Automated checks cover sensor selection, parsing, missing/invalid data, GPU identity
ambiguity, persistent helper requests, timeout, cancellation, and queued delivery.
CI builds the helper and checks its selection policy without requiring hardware sensors.

Sources and licenses:
- https://github.com/LibreHardwareMonitor/LibreHardwareMonitor/tree/v0.9.6 (MPL-2.0)
- https://github.com/LibreHardwareMonitor/LibreHardwareMonitor/blob/v0.9.6/THIRD-PARTY-NOTICES.txt
- https://www.nuget.org/packages/LibreHardwareMonitorLib/0.9.6
