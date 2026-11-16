# Third-Party Notices

System Monitor's own source code is licensed under the [MIT License](LICENSE). It builds on the following components, which keep their own licenses.

## Qt

- **Used for:** the desktop application (Qt Core, Qt Gui and Qt Widgets)
- **License:** GNU LGPL v3 (open source edition), or a commercial Qt license
- **Source:** <https://www.qt.io/download-open-source>

Qt is linked dynamically. `windeployqt` copies the Qt libraries next to the executable, so they can be replaced with other compatible builds.

## LibreHardwareMonitor

- **Used for:** CPU and GPU temperature readings in `helpers/TemperatureHelper`
- **Version:** LibreHardwareMonitorLib 0.9.6 (NuGet)
- **License:** Mozilla Public License 2.0
- **Source:** <https://github.com/LibreHardwareMonitor/LibreHardwareMonitor/tree/v0.9.6>
- **Its own third-party notices:** <https://github.com/LibreHardwareMonitor/LibreHardwareMonitor/blob/v0.9.6/THIRD-PARTY-NOTICES.txt>

The temperature helper is a separate executable that uses the unmodified library from NuGet. The full list of the helper's NuGet dependencies and versions is in `helpers/TemperatureHelper/packages.lock.json`.

## .NET

- **Used for:** the self-contained temperature helper runtime
- **License:** MIT
- **Source:** <https://github.com/dotnet/runtime>
