#include "GpuMonitor.h"
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <thread>

int main(int argc, char**)
{
    auto require = [](bool condition, const char* message) {
        if (!condition) { std::cerr << message << '\n'; std::exit(1); }
    };
    const auto values = aggregateGpuUsage({
        {L"pid_1_luid_0x00000000_0x00000001_phys_0_eng_0_engtype_3D", 30},
        {L"pid_2_luid_0x00000000_0x00000001_phys_0_eng_0_engtype_3D", 40},
        {L"pid_1_luid_0x00000000_0x00000001_phys_0_eng_1_engtype_Copy", 50},
        {L"pid_3_luid_0x00000001_0x00000002_phys_0_eng_0_engtype_VideoEncode", 10},
        {L"pid_4_luid_0x00000000_0x00000001_phys_1_eng_0_engtype_3D", 90},
        {L"malformed", 90},
        {L"pid_5_luid_0x00000000_0x00000001_phys_0_eng_0_engtype_3D", -10},
        {L"pid_5_luid_0x00000000_0x00000001_phys_0_eng_0_engtype_3D", std::numeric_limits<double>::quiet_NaN()}});
    require(values.size() == 3 && values.at({1, 0}).percent == 70
            && values.at({1, 0}).busiestEngine == L"3D", "Engine aggregation failed");
    require(values.at({(std::uint64_t{1} << 32) | 2, 0}).percent == 10
            && values.at({1, 1}).percent == 90, "Adapters or physical nodes mixed");
    const auto capped = aggregateGpuUsage({
        {L"pid_1_luid_0x0_0x1_phys_0_eng_0_engtype_3D", 80},
        {L"pid_2_luid_0x0_0x1_phys_0_eng_0_engtype_3D", 70}});
    require(capped.at({1, 0}).percent == 100, "Over-100 engine usage not capped");
    require(aggregateGpuUsage({}).empty(), "Missing counters treated as idle");

    GpuMonitor monitor;
    bool live = false;
    for (int attempt = 0; attempt < 3; ++attempt) {
        const auto snapshot = monitor.sample();
        for (const auto& gpu : snapshot.adapters) {
            require(!gpu.name.empty(), "Unnamed adapter");
            if (gpu.percent) {
                require(gpu.status == GpuReadingStatus::Live && std::isfinite(*gpu.percent)
                        && *gpu.percent >= 0 && *gpu.percent <= 100, "Invalid live usage");
                live = true;
                std::wcout << gpu.name << L": " << *gpu.percent << L"%, " << gpu.busiestEngine << L'\n';
            }
        }
        if (live || attempt == 2) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(1100));
    }
    if (argc > 1) require(live, "Local GPU counters did not produce a reading");
    if (!live) std::cout << "GPU counters unavailable on this machine; fallback supported\n";
    std::cout << "GPU aggregation and collection checks passed\n";
}
