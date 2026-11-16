# Setup

## Prerequisites

- **Windows 10 or 11 (x64).** The app reads Windows performance counters and Win32 APIs directly, so it does not build on other platforms.
- **Qt 6** (tested with 6.8 and 6.12) with either the MinGW 64-bit kit or an MSVC 2022 64-bit kit. The Qt online installer also provides CMake, Ninja and the matching MinGW compiler under `Tools`.
- **.NET 8 SDK** to build the optional temperature helper. The helper is published self-contained, so running it does not need .NET.

All commands below run from the project root in PowerShell.

## Build with the PowerShell script (MinGW)

```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1
.\build\SystemMonitor.exe
```

`build.ps1` configures and builds a Release build in `build\` with Ninja and Qt's own MinGW compiler, then runs `windeployqt` so the executable runs outside Qt Creator.

By default the script looks in `C:\Qt`, picks the newest Qt version that has a `mingw_64` kit, and uses the `Tools\mingw*_64` compiler that kit was built with. Point it somewhere else, or pin the versions, with:

```powershell
.\build.ps1 -QtRoot 'D:\Qt' -QtVersion '6.8.3' -MinGW 'mingw1310_64'
```

## Build with Qt Creator

Open `CMakeLists.txt` in Qt Creator, select a Desktop Qt 6 MinGW 64-bit or MSVC 64-bit kit, then build and run. Qt Creator puts the executable in its own build directory, so see [Temperature helper](#temperature-helper) for where the helper goes.

## Build with Visual Studio (MSVC)

This matches the CI build:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH=C:\Qt\6.8.3\msvc2022_64
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

Run `windeployqt` on `build\Release\SystemMonitor.exe` to start it outside Visual Studio.

## Run the tests

```powershell
ctest --test-dir build --output-on-failure
```

If `ctest` is not on your `PATH`, use the copy that ships with Qt, for example `C:\Qt\Tools\CMake_64\bin\ctest.exe`. Add `-C Release` for Visual Studio builds.

## Temperature helper

CPU and GPU temperatures come from a small C# helper built on LibreHardwareMonitor. Build it once:

```powershell
powershell -ExecutionPolicy Bypass -File .\build-temperatures.ps1
```

The script publishes a self-contained helper to `build\temperature-helper\` and runs its sensor-selection self-test. It uses `build-tools\dotnet\dotnet.exe` if that exists, otherwise the `dotnet` on your `PATH`. NuGet package versions are locked in `helpers/TemperatureHelper/packages.lock.json`.

The app looks for the helper in a `temperature-helper` folder next to `SystemMonitor.exe`. For a Qt Creator or Visual Studio build, publish it there instead:

```powershell
.\build-temperatures.ps1 -OutputDirectory 'C:\path\to\build\Release\temperature-helper'
```

The C++ app builds and runs without the helper. Temperature labels then show Unavailable, and hovering them explains why.

## Enable CPU temperatures

GPU temperatures work in a normal launch. Most CPU temperature sensors also need:

1. The signed [PawnIO](https://github.com/namazso/PawnIO.Setup) driver, which LibreHardwareMonitor uses to read CPU sensors. Install it yourself if you want CPU temperatures. The app never installs drivers.
2. Administrator access. Right-click `build\SystemMonitor.exe` and choose **Run as administrator**, or use the shortcut below.

The app never requests elevation by itself. Its header says whether it is running with administrator access.

## Optional shortcuts

After building, create Desktop and Start menu shortcuts that launch the app as administrator:

```powershell
powershell -ExecutionPolicy Bypass -File .\setup-shortcuts.ps1
```

Both shortcuts point at `build\SystemMonitor.exe`, so rebuilding keeps them working. Windows shows its normal UAC prompt on each launch. To pin the app, right-click the Start menu shortcut and choose **Pin to Start**.

## Settings

The selected theme is stored with `QSettings` under `HKEY_CURRENT_USER\Software\SystemMonitor\System Monitor`.
