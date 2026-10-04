#include "MonitoringService.h"
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QTimer>
#include <atomic>
#include <cstdlib>
#include <iostream>

namespace {
void require(bool condition, const char* message)
{
    if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    std::atomic<int> calls{0};
    std::atomic<bool> onWorker{false};
    int heartbeats = 0;
    int deliveries = 0;
    bool deliveredOnMain = false;
    QEventLoop loop;
    QTimer heartbeat;
    QObject::connect(&heartbeat, &QTimer::timeout, [&] { ++heartbeats; });
    heartbeat.start(10);
    MonitoringService service(nullptr, [&](const auto& cancelled) -> ProcessSample {
        const int call = ++calls;
        onWorker = QThread::currentThread() != app.thread();
        QElapsedTimer elapsed;
        elapsed.start();
        while (elapsed.elapsed() < 150 && !cancelled()) QThread::msleep(2);
        if (call == 1) return std::nullopt;
        return std::vector<ProcessInfo>{{42, L"test.exe", 1024}};
    }, [] { return GpuSnapshot{}; });
    QObject::connect(&service, &MonitoringService::sampleReady, &app,
        [&](const ProcessSample& sample) {
            deliveredOnMain = QThread::currentThread() == app.thread();
            ++deliveries;
            require(deliveries == 1 ? !sample : sample && sample->at(0).id == 42,
                    "Failure/recovery snapshot not delivered");
            loop.quit();
        });
    bool timedOut = false;
    QTimer timeout;
    timeout.setSingleShot(true);
    QObject::connect(&timeout, &QTimer::timeout, [&] { timedOut = true; loop.quit(); });
    timeout.start(3000);
    require(service.requestRefresh(), "Initial request rejected");
    for (int i = 0; i < 100; ++i)
        require(!service.requestRefresh(), "Overlapping request queued");
    loop.exec();
    require(!timedOut && deliveries == 1 && calls == 1, "First snapshot failed or requests accumulated");
    require(onWorker && deliveredOnMain && heartbeats >= 3, "GUI blocked or thread ownership incorrect");
    require(service.requestRefresh(), "Retry after failure rejected");
    loop.exec();
    require(!timedOut && deliveries == 2 && calls == 2, "Recovery failed");
    service.stop();
    service.stop();
    require(!service.requestRefresh(), "Stopped service accepted a request");

    // Shut down during a long collection, rather than only while idle.
    std::atomic<bool> started{false};
    std::atomic<bool> cancelled{false};
    MonitoringService active(nullptr, [&](const auto& cancel) -> ProcessSample {
        started = true;
        while (!cancel()) QThread::msleep(2);
        cancelled = true;
        return std::nullopt;
    }, [] { return GpuSnapshot{}; });
    int lateDeliveries = 0;
    QObject::connect(&active, &MonitoringService::sampleReady, &app,
                     [&](const ProcessSample&) { ++lateDeliveries; });
    active.requestRefresh();
    QElapsedTimer startWait;
    startWait.start();
    while (!started && startWait.elapsed() < 3000) {
        QCoreApplication::processEvents();
        QThread::msleep(1);
    }
    require(started, "Worker did not start");
    QElapsedTimer shutdown;
    shutdown.start();
    active.stop();
    QCoreApplication::processEvents();
    require(cancelled && shutdown.elapsed() < 1000 && lateDeliveries == 0,
            "Active shutdown failed or delivered after stopping");

    // Exercise the real Windows collector through the same queued connection.
    MonitoringService live;
    QObject::connect(&live, &MonitoringService::sampleReady, &app,
                     [&](const ProcessSample& sample) {
        require(sample && !sample->empty(), "Live worker collection failed");
        loop.quit();
    });
    bool gpuDelivered = false;
    QObject::connect(&live, &MonitoringService::gpuReady, &app, [&](const GpuSnapshot&) {
        gpuDelivered = QThread::currentThread() == app.thread();
    });
    live.requestRefresh();
    loop.exec();
    require(!timedOut, "Live worker timed out");
    require(gpuDelivered, "GPU snapshot not delivered on the GUI thread");
    live.stop();
    std::cout << "Worker delivery, responsiveness, coalescing, recovery, and shutdown passed\n";
}
