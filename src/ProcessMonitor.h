#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

struct ProcessInfo {
    std::uint32_t id;
    std::wstring name;
    std::optional<std::uint64_t> workingSetBytes;
};

class ProcessMonitor {
public:
    // A failed enumeration is different from a valid, empty process list.
    std::optional<std::vector<ProcessInfo>> sample(const std::function<bool()>& cancelled = {}) const;
};
