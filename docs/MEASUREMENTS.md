# Measurements

How each reading is calculated, and where it can differ from Task Manager.

## CPU usage

CPU usage is busy CPU time divided by total CPU time over the sampling interval, using `GetSystemTimes`. Windows kernel time includes idle time, so idle is subtracted once.

- Rapid refreshes with no new counter ticks keep the last valid percentage.
- A failed read or a counter reset starts a fresh baseline.
- On systems with more than 64 logical processors, `GetSystemTimes` covers the calling thread's processor group.
- This is a time-based percentage, so it can differ from Task Manager's frequency-adjusted "Processor Utility".

## CPU history

The graph uses a fixed 0 to 100% scale and a rolling 60-second window.

- Samples are stamped with a monotonic clock, so a late timer tick still lands in the right place.
- Rapid Refresh clicks do not add extra points.
- Missing or invalid readings and long pauses leave a gap in the line, and old points expire automatically.

## Memory

- **System RAM** is physical memory in use out of total physical memory.
- **Process memory** is the working set (resident physical memory), which can include shared pages. It can differ from Task Manager's default Memory column, which shows private memory.
- Protected or exited processes show Unavailable rather than zero. Sorting, selection and scroll position are kept across refreshes.

## GPU usage

Hardware adapters are found through DXGI and matched to Windows GPU Engine performance counters by adapter LUID and physical node index. Software adapters are excluded.

- Process contributions are summed within each engine, and each GPU card shows its busiest engine, capped at 100%. Percentages from independent engines are not added together, so video encode or compute load stays visible next to 3D load.
- The worker owns a persistent PDH query and uses English counter paths, so it works on any Windows display language.
- The first read sets a baseline. Manual refreshes less than 900 ms apart reuse the last GPU sample.
- Missing or invalid counters show Unavailable and are retried. Valid idle counters show 0%.
- Known GPU names stay visible if adapter enumeration fails temporarily.

References:

- [GPUs in the Task Manager](https://devblogs.microsoft.com/directx/gpus-in-the-task-manager/) (DirectX Developer Blog)
- [PdhGetFormattedCounterArrayW](https://learn.microsoft.com/en-us/windows/win32/api/pdh/nf-pdh-pdhgetformattedcounterarrayw) (Microsoft Learn)

## Temperatures

Readings come from [LibreHardwareMonitor](https://github.com/LibreHardwareMonitor/LibreHardwareMonitor) 0.9.6 through the helper described in [Architecture](ARCHITECTURE.md#temperature-helper).

- **CPU:** Intel "CPU Package" or AMD "Tctl/Tdie" is preferred. If neither exists, the hottest core is shown with a "Hottest CPU core" label. With several CPU packages, the hottest reading is shown.
- **GPU:** "GPU Core" is used. Hotspot and memory temperatures are not.
- **Matching GPUs to cards:** A temperature is assigned to a GPU card only when the GPU's name is unique in both the usage and temperature snapshots. Duplicate model names and linked GPU nodes show Unavailable temperature rather than risk showing the wrong GPU's reading. Usage still works.
- **Validation:** Null, non-numeric and out-of-range readings (outside 0 to 150 °C) show Unavailable.
- ACPI thermal zones are not used as a substitute for CPU package temperature.

See [Setup](SETUP.md#enable-cpu-temperatures) for the driver and administrator access that CPU sensors need.

## Test coverage

Automated tests cover the calculations above without needing specific hardware: CPU counter arithmetic and resets, history retention and gaps, GPU engine aggregation across multiple adapters and nodes, invalid readings, sensor selection and parsing, GPU identity ambiguity, the helper protocol with timeouts and cancellation, and queued delivery from the worker. Tests that read live counters accept unavailable hardware on CI runners.
