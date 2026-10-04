#include "SystemMonitor.h"
#include <cmath>
#include <iostream>

// Integration check: verify the live Windows measurement is usable by the UI.
int main()
{
    const auto snapshot = SystemMonitor{}.sample();
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
    return 0;
}
