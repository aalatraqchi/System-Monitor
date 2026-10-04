#pragma once

#include "TemperatureSnapshot.h"
#include <QElapsedTimer>
#include <QProcess>
#include <QStringList>
#include <functional>

// Own this object on the worker. The helper isolates .NET and sensor API failures.
class TemperatureMonitor {
public:
    using CancelCheck = std::function<bool()>;
    explicit TemperatureMonitor(QString helperPath = {}, QStringList arguments = {"--stdio"},
                                int timeoutMs = 2500);
    ~TemperatureMonitor();
    TemperatureSnapshot sample(const CancelCheck& cancelled);
private:
    void stopHelper();
    TemperatureSnapshot unavailable(const QString& message);
    QString helperPath_;
    QStringList arguments_;
    int timeoutMs_;
    QProcess helper_;
    QElapsedTimer retryTimer_;
    QString failureMessage_;
};
