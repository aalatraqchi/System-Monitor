#pragma once

#include "SystemSnapshot.h"

// Collects system data without knowing anything about the window or its widgets.
class SystemMonitor {
public:
    SystemSnapshot sample() const;
};
