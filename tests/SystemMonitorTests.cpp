#include "SystemMonitor.h"
#include <cmath>
#include <iostream>
#include <chrono>
#include <thread>

// Integration check: verify the live Windows measurement is usable by the UI.
int main()
{
    SystemMonitor monitor;
    const auto snapshot = monitor.sample();
    if (!snapshot.memory) {
        std::cerr << "Windows physical memory measurement unavailable\n";
        return 1;
    }
    const auto& memory = *snapshot.memory;
    if (memory.totalBytes == 0 || memory.usedBytes > memory.totalBytes
        || !std::isfinite(memory.percent) || memory.percent < 0.0 || memory.percent > 100.0) {
        std::cerr << "Physical memory measurement is outside valid bounds\n";
        return 1;
    }
    std::cout << "RAM: " << memory.percent << "% (" << memory.usedBytes
              << " / " << memory.totalBytes << " bytes)\n";
    if (snapshot.cpuPercent) {
        std::cerr << "First CPU sample should establish a baseline\n";
        return 1;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    const auto next = monitor.sample();
    if (!next.cpuPercent || !std::isfinite(*next.cpuPercent)
        || *next.cpuPercent < 0.0 || *next.cpuPercent > 100.0) {
        std::cerr << "Second CPU sample should contain a valid usage percentage\n";
        return 1;
    }
    std::cout << "CPU: " << *next.cpuPercent << "%\n";
    return 0;
}
