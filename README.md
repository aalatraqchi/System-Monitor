# System Monitor

A Windows desktop application using C++17 and Qt Widgets.

## Current milestone
RAM shows live physical memory usage as a percentage and used/total GiB, refreshed
once per second. Refresh also samples immediately. CPU remains a placeholder.

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
- `SystemMonitor`: reads Windows physical memory independently of the UI.

Qt owns child widgets through their parents. Layouts handle window resizing.

## Next steps
1. Add CPU sampling using the existing one-second refresh timer.
2. Add a sortable process table.
3. Add CPU history, background process collection, and UI polish.

## Version control
See [DEVELOPMENT.md](DEVELOPMENT.md) for the branch, commit, and pull-request workflow.
