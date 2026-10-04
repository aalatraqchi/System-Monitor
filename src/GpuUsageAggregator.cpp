#include "GpuUsageAggregator.h"
#include <algorithm>
#include <cmath>
#include <regex>
#include <tuple>

bool GpuKey::operator<(const GpuKey& other) const
{
    return std::tie(adapterId, physicalIndex) < std::tie(other.adapterId, other.physicalIndex);
}

std::map<GpuKey, GpuUsage> aggregateGpuUsage(const std::vector<GpuEngineSample>& samples)
{
    static const std::wregex instancePattern(
        LR"(^pid_[0-9]+_luid_0x([0-9a-fA-F]{1,8})_0x([0-9a-fA-F]{1,8})_phys_([0-9]+)_eng_([0-9]+)_engtype_(.+)$)");
    struct Engine { double total = 0; std::wstring name; };
    std::map<GpuKey, std::map<unsigned, Engine>> engines;
    for (const auto& sample : samples) {
        if (!std::isfinite(sample.percent) || sample.percent < 0) continue;
        std::wsmatch match;
        if (!std::regex_match(sample.instance, match, instancePattern)) continue;
        try {
            const auto high = std::stoull(match[1].str(), nullptr, 16);
            const auto low = std::stoull(match[2].str(), nullptr, 16);
            const auto physical = std::stoul(match[3].str());
            const auto engineId = std::stoul(match[4].str());
            auto& engine = engines[{(high << 32) | low, static_cast<unsigned>(physical)}]
                                  [static_cast<unsigned>(engineId)];
            // Saturation avoids over-100 results from counter rounding or overlap.
            engine.total = std::min(100.0, engine.total + sample.percent);
            engine.name = match[5].str();
        } catch (const std::exception&) {
            // A malformed counter instance should not hide the other GPUs.
        }
    }
    std::map<GpuKey, GpuUsage> result;
    for (const auto& gpu : engines) {
        GpuUsage usage{0, {}};
        for (const auto& engine : gpu.second)
            if (usage.busiestEngine.empty() || engine.second.total > usage.percent)
                usage = {engine.second.total, engine.second.name};
        result.emplace(gpu.first, std::move(usage));
    }
    return result;
}
