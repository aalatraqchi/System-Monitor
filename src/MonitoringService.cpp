#include "MonitoringService.h"
#include <utility>

namespace {
class MonitoringWorker : public QObject {
    Q_OBJECT
public:
    explicit MonitoringWorker(MonitoringService::Sampler sampler, MonitoringService::GpuSampler gpuSampler)
        : sampler_(std::move(sampler)), gpuSampler_(std::move(gpuSampler)) {}
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
        if (!cancelled()) emit sampleReady(sample);
    }
signals:
    void sampleReady(const ProcessSample& sample);
    void gpuReady(const GpuSnapshot& sample);
private:
    MonitoringService::Sampler sampler_;
    MonitoringService::GpuSampler gpuSampler_;
    std::unique_ptr<GpuMonitor> gpuMonitor_;
};
}

MonitoringService::MonitoringService(QObject* parent, Sampler sampler, GpuSampler gpuSampler)
    : QObject(parent)
{
    qRegisterMetaType<ProcessSample>();
    qRegisterMetaType<GpuSnapshot>();
    if (!sampler) sampler = [](const CancelCheck& cancelled) {
        return ProcessMonitor{}.sample(cancelled);
    };
    auto* worker = new MonitoringWorker(std::move(sampler), std::move(gpuSampler));
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
    workerThread_.setObjectName("Process and GPU collection");
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
