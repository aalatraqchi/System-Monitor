#pragma once

#include "ProcessMonitor.h"
#include "GpuMonitor.h"
#include "TemperatureSnapshot.h"
#include <QMetaType>
#include <QObject>
#include <QThread>
#include <functional>

using ProcessSample = std::optional<std::vector<ProcessInfo>>;
Q_DECLARE_METATYPE(ProcessSample)
Q_DECLARE_METATYPE(GpuSnapshot)
Q_DECLARE_METATYPE(TemperatureSnapshot)

// This controller lives on the GUI thread; its worker owns the slow collectors.
class MonitoringService : public QObject {
    Q_OBJECT
public:
    using CancelCheck = std::function<bool()>;
    using Sampler = std::function<ProcessSample(const CancelCheck&)>;
    using GpuSampler = std::function<GpuSnapshot()>;
    using TemperatureSampler = std::function<TemperatureSnapshot(const CancelCheck&)>;
    explicit MonitoringService(QObject* parent = nullptr, Sampler sampler = {}, GpuSampler gpuSampler = {},
                               TemperatureSampler temperatureSampler = {});
    ~MonitoringService() override;
    bool requestRefresh();
    void stop();

signals:
    void sampleReady(const ProcessSample& sample);
    void gpuReady(const GpuSnapshot& sample);
    void temperaturesReady(const TemperatureSnapshot& sample);
    void collectRequested();

private:
    QThread workerThread_;
    bool busy_ = false;
    bool stopping_ = false;
};
