#include "GpuMonitor.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <dxgi.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <chrono>

namespace {
struct ComRelease {
    template<class T> void operator()(T* object) const { if (object) object->Release(); }
};

GpuSnapshot enumerateAdapters()
{
    GpuSnapshot snapshot;
    IDXGIFactory1* rawFactory = nullptr;
    if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&rawFactory)))) {
        snapshot.enumerationAvailable = false;
        return snapshot;
    }
    std::unique_ptr<IDXGIFactory1, ComRelease> factory(rawFactory);
    for (UINT index = 0; ; ++index) {
        IDXGIAdapter1* rawAdapter = nullptr;
        const auto status = factory->EnumAdapters1(index, &rawAdapter);
        if (status == DXGI_ERROR_NOT_FOUND) break;
        if (FAILED(status)) { snapshot.enumerationAvailable = false; break; }
        std::unique_ptr<IDXGIAdapter1, ComRelease> adapter(rawAdapter);
        DXGI_ADAPTER_DESC1 description{};
        if (FAILED(adapter->GetDesc1(&description))) { snapshot.enumerationAvailable = false; continue; }
        if (description.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
        const auto id = (static_cast<std::uint64_t>(static_cast<std::uint32_t>(description.AdapterLuid.HighPart)) << 32)
                      | description.AdapterLuid.LowPart;
        snapshot.adapters.push_back({{id, 0}, description.Description, std::nullopt, {}, GpuReadingStatus::Unavailable});
    }
    return snapshot;
}

std::optional<std::vector<GpuEngineSample>> readEngines(PDH_HCOUNTER counter)
{
    // Instance counts can change between the size query and the data read.
    for (int attempt = 0; attempt < 3; ++attempt) {
        DWORD bytes = 0, count = 0;
        const auto sizeStatus = PdhGetFormattedCounterArrayW(counter, PDH_FMT_DOUBLE, &bytes, &count, nullptr);
        if (sizeStatus != static_cast<PDH_STATUS>(PDH_MORE_DATA) || bytes == 0 || bytes > 64 * 1024 * 1024) return std::nullopt;
        std::vector<PDH_FMT_COUNTERVALUE_ITEM_W> buffer(
            (bytes + sizeof(PDH_FMT_COUNTERVALUE_ITEM_W) - 1) / sizeof(PDH_FMT_COUNTERVALUE_ITEM_W));
        const auto status = PdhGetFormattedCounterArrayW(counter, PDH_FMT_DOUBLE, &bytes, &count, buffer.data());
        if (status == static_cast<PDH_STATUS>(PDH_MORE_DATA)) continue;
        if (status != ERROR_SUCCESS) return std::nullopt;
        std::vector<GpuEngineSample> samples;
        for (DWORD i = 0; i < count; ++i) {
            const auto& value = buffer[i];
            if (value.szName && (value.FmtValue.CStatus == PDH_CSTATUS_VALID_DATA
                                || value.FmtValue.CStatus == PDH_CSTATUS_NEW_DATA))
                samples.push_back({value.szName, value.FmtValue.doubleValue});
        }
        return samples;
    }
    return std::nullopt;
}
}

struct GpuMonitor::State {
    PDH_HQUERY query = nullptr;
    PDH_HCOUNTER counter = nullptr;
    bool baseline = false;
    GpuSnapshot cached;
    std::chrono::steady_clock::time_point lastSample{};
    ~State() { reset(); }
    void reset() {
        if (query) PdhCloseQuery(query);
        query = nullptr;
        counter = nullptr;
        baseline = false;
    }
    bool open() {
        if (query) return true;
        if (PdhOpenQueryW(nullptr, 0, &query) != ERROR_SUCCESS) { reset(); return false; }
        // English counter names work even when Windows uses another display language.
        if (PdhAddEnglishCounterW(query, L"\\GPU Engine(*)\\Utilization Percentage", 0, &counter) != ERROR_SUCCESS) {
            reset(); return false;
        }
        return true;
    }
};

GpuMonitor::GpuMonitor() : state_(std::make_unique<State>()) {}
GpuMonitor::~GpuMonitor() = default;

GpuSnapshot GpuMonitor::sample()
{
    auto& state = *state_;
    const auto now = std::chrono::steady_clock::now();
    // Avoid taking two rate-counter readings milliseconds apart on manual Refresh.
    if (state.lastSample != std::chrono::steady_clock::time_point{}
        && now - state.lastSample < std::chrono::milliseconds(900)) return state.cached;
    state.lastSample = now;
    auto snapshot = enumerateAdapters();
    if (!state.open() || PdhCollectQueryData(state.query) != ERROR_SUCCESS) {
        state.reset();
        state.cached = snapshot;
        return snapshot;
    }
    if (!state.baseline) {
        state.baseline = true;
        for (auto& adapter : snapshot.adapters) adapter.status = GpuReadingStatus::Waiting;
    } else if (const auto samples = readEngines(state.counter)) {
        const auto usages = aggregateGpuUsage(*samples);
        const auto baseAdapters = snapshot.adapters;
        for (const auto& usage : usages) {
            for (const auto& base : baseAdapters) {
                if (base.key.adapterId != usage.first.adapterId) continue;
                auto reading = base;
                reading.key = usage.first;
                reading.percent = usage.second.percent;
                reading.busiestEngine = usage.second.busiestEngine;
                reading.status = GpuReadingStatus::Live;
                if (usage.first.physicalIndex == 0) {
                    for (auto& adapter : snapshot.adapters)
                        if (adapter.key.adapterId == base.key.adapterId) adapter = reading;
                } else {
                    // Linked adapters can expose several physical GPUs under one LUID.
                    reading.name += L" (node " + std::to_wstring(usage.first.physicalIndex) + L")";
                    snapshot.adapters.push_back(std::move(reading));
                }
            }
        }
    }
    state.cached = snapshot;
    return snapshot;
}
