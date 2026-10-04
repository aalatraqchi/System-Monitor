#include "ProcessSamplingService.h"
#include <utility>

namespace {
class ProcessWorker : public QObject {
    Q_OBJECT
public:
    explicit ProcessWorker(ProcessSamplingService::Sampler sampler)
        : sampler_(std::move(sampler)) {}
    void collect()
    {
        const auto cancelled = [] { return QThread::currentThread()->isInterruptionRequested(); };
        if (cancelled()) return;
        const auto sample = sampler_(cancelled);
        if (!cancelled()) emit sampleReady(sample);
    }
signals:
    void sampleReady(const ProcessSample& sample);
private:
    ProcessSamplingService::Sampler sampler_;
};
}

ProcessSamplingService::ProcessSamplingService(QObject* parent, Sampler sampler)
    : QObject(parent)
{
    qRegisterMetaType<ProcessSample>();
    if (!sampler) sampler = [](const CancelCheck& cancelled) {
        return ProcessMonitor{}.sample(cancelled);
    };
    auto* worker = new ProcessWorker(std::move(sampler));
    worker->moveToThread(&workerThread_);
    connect(&workerThread_, &QThread::finished, worker, &QObject::deleteLater);
    connect(this, &ProcessSamplingService::collectRequested,
            worker, &ProcessWorker::collect, Qt::QueuedConnection);
    // Qt delivers a copied snapshot on this controller's thread, never on the worker.
    connect(worker, &ProcessWorker::sampleReady, this, [this](const ProcessSample& sample) {
        if (stopping_) return;
        busy_ = false;
        emit sampleReady(sample);
    }, Qt::QueuedConnection);
    workerThread_.setObjectName("Process collection");
    workerThread_.start();
}

ProcessSamplingService::~ProcessSamplingService()
{
    stop();
}

bool ProcessSamplingService::requestRefresh()
{
    Q_ASSERT(QThread::currentThread() == thread());
    // Ignore overlapping timer/manual requests instead of building a queue.
    if (busy_ || stopping_) return false;
    busy_ = true;
    emit collectRequested();
    return true;
}

void ProcessSamplingService::stop()
{
    Q_ASSERT(QThread::currentThread() == thread());
    if (stopping_) return;
    stopping_ = true;
    workerThread_.requestInterruption();
    workerThread_.quit();
    // Collection checks interruption between processes; handles close before exit.
    workerThread_.wait();
}

#include "ProcessSamplingService.moc"
