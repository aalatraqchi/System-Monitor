# System Monitor

A Windows desktop application using C++17 and Qt Widgets.

## Current milestone
The base window displays CPU and RAM placeholders. Refresh calls the monitoring
interface; real measurements will be implemented next. No fake usage is shown.

## Build and run
Open `CMakeLists.txt` in Qt Creator and select **Desktop Qt 6.12.0 MinGW 64-bit**.
Build and run using Qt Creator, or run this from PowerShell:

```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1
.\build\SystemMonitor.exe
```

The script uses the Qt installation at `E:\QTCreator`. Pass `-QtRoot` if it moves.
It builds a release executable and copies the required Qt runtime dependencies.

## Code map
- `main.cpp`: starts Qt and opens the window.
- `MainWindow`: builds the UI and displays measurements.
- `SystemSnapshot`: stores available measurements.
- `SystemMonitor`: collects data independently of the UI (currently a stub).

Qt owns child widgets through their parents. Layouts handle window resizing.

## Next steps
1. Read RAM usage through the Windows API.
2. Add CPU sampling and a one-second refresh timer.
3. Add a sortable process table.
4. Add CPU history, background process collection, and UI polish.

## Version control
See [DEVELOPMENT.md](DEVELOPMENT.md) for the branch, commit, and pull-request workflow.
