#pragma once

#include "ProcessMonitor.h"
#include <QMetaType>
#include <QObject>
#include <QThread>
#include <functional>

using ProcessSample = std::optional<std::vector<ProcessInfo>>;
Q_DECLARE_METATYPE(ProcessSample)

// This controller lives on the GUI thread; its worker owns process collection.
class ProcessSamplingService : public QObject {
    Q_OBJECT
public:
    using CancelCheck = std::function<bool()>;
    using Sampler = std::function<ProcessSample(const CancelCheck&)>;
    explicit ProcessSamplingService(QObject* parent = nullptr, Sampler sampler = {});
    ~ProcessSamplingService() override;
    bool requestRefresh();
    void stop();

signals:
    void sampleReady(const ProcessSample& sample);
    void collectRequested();

private:
    QThread workerThread_;
    bool busy_ = false;
    bool stopping_ = false;
};
