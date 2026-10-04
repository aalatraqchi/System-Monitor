#include "MonitoringService.h"
#include "TemperatureMonitor.h"
#include <utility>

namespace {
class MonitoringWorker : public QObject {
    Q_OBJECT
public:
    explicit MonitoringWorker(MonitoringService::Sampler sampler, MonitoringService::GpuSampler gpuSampler,
                              MonitoringService::TemperatureSampler temperatureSampler)
        : sampler_(std::move(sampler)), gpuSampler_(std::move(gpuSampler)),
          temperatureSampler_(std::move(temperatureSampler)) {}
    void collect()
    {
        const auto cancelled = [] { return QThread::currentThread()->isInterruptionRequested(); };
        if (cancelled()) return;
        {
            // The persistent PDH query is created and destroyed on this worker.
            if (!gpuSampler_ && !gpuMonitor_) gpuMonitor_ = std::make_unique<GpuMonitor>();
            const auto gpu = gpuSampler_ ? gpuSampler_() : gpuMonitor_->sample();
            if (cancelled()) return;
            emit gpuReady(gpu);
        }
        const auto sample = sampler_(cancelled);
        if (cancelled()) return;
        if (!temperatureSampler_ && !temperatureMonitor_) temperatureMonitor_ = std::make_unique<TemperatureMonitor>();
        const auto temperatures = temperatureSampler_ ? temperatureSampler_(cancelled)
                                                       : temperatureMonitor_->sample(cancelled);
        if (!cancelled()) {
            emit temperaturesReady(temperatures);
            // Release the request guard only after all collectors finish.
            emit sampleReady(sample);
        }
    }
signals:
    void sampleReady(const ProcessSample& sample);
    void gpuReady(const GpuSnapshot& sample);
    void temperaturesReady(const TemperatureSnapshot& sample);
private:
    MonitoringService::Sampler sampler_;
    MonitoringService::GpuSampler gpuSampler_;
    MonitoringService::TemperatureSampler temperatureSampler_;
    std::unique_ptr<GpuMonitor> gpuMonitor_;
    std::unique_ptr<TemperatureMonitor> temperatureMonitor_;
};
}

MonitoringService::MonitoringService(QObject* parent, Sampler sampler, GpuSampler gpuSampler,
                                     TemperatureSampler temperatureSampler)
    : QObject(parent)
{
    qRegisterMetaType<ProcessSample>();
    qRegisterMetaType<GpuSnapshot>();
    qRegisterMetaType<TemperatureSnapshot>();
    if (!sampler) sampler = [](const CancelCheck& cancelled) {
        return ProcessMonitor{}.sample(cancelled);
    };
    auto* worker = new MonitoringWorker(std::move(sampler), std::move(gpuSampler), std::move(temperatureSampler));
    worker->moveToThread(&workerThread_);
    connect(&workerThread_, &QThread::finished, worker, &QObject::deleteLater);
    connect(this, &MonitoringService::collectRequested,
            worker, &MonitoringWorker::collect, Qt::QueuedConnection);
    // Qt delivers a copied snapshot on this controller's thread, never on the worker.
    connect(worker, &MonitoringWorker::sampleReady, this, [this](const ProcessSample& sample) {
        if (stopping_) return;
        busy_ = false;
        emit sampleReady(sample);
    }, Qt::QueuedConnection);
    connect(worker, &MonitoringWorker::gpuReady, this, [this](const GpuSnapshot& sample) {
        if (!stopping_) emit gpuReady(sample);
    }, Qt::QueuedConnection);
    connect(worker, &MonitoringWorker::temperaturesReady, this, [this](const TemperatureSnapshot& sample) {
        if (!stopping_) emit temperaturesReady(sample);
    }, Qt::QueuedConnection);
    workerThread_.setObjectName("Process, GPU, and temperature collection");
    workerThread_.start();
}

MonitoringService::~MonitoringService()
{
    stop();
}

bool MonitoringService::requestRefresh()
{
    Q_ASSERT(QThread::currentThread() == thread());
    // Ignore overlapping timer/manual requests instead of building a queue.
    if (busy_ || stopping_) return false;
    busy_ = true;
    emit collectRequested();
    return true;
}

void MonitoringService::stop()
{
    Q_ASSERT(QThread::currentThread() == thread());
    if (stopping_) return;
    stopping_ = true;
    workerThread_.requestInterruption();
    workerThread_.quit();
    // Collection checks interruption between processes; handles close before exit.
    workerThread_.wait();
}

#include "MonitoringService.moc"
