#include "SystemMonitor.h"

SystemSnapshot SystemMonitor::sample() const
{
    // The base app has no Windows API measurements yet.
    // Real RAM and CPU collection will be added here in the next milestones.
    return {};
}
