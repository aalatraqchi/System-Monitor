#include "TemperatureMonitor.h"
#include <QCoreApplication>
#include <QFileInfo>

TemperatureMonitor::TemperatureMonitor(QString helperPath, QStringList arguments, int timeoutMs)
    : helperPath_(helperPath.isEmpty() ? QCoreApplication::applicationDirPath()
        + "/temperature-helper/TemperatureHelper.exe" : std::move(helperPath)),
      arguments_(std::move(arguments)), timeoutMs_(timeoutMs)
{
    helper_.setProcessChannelMode(QProcess::SeparateChannels);
    helper_.setStandardErrorFile(QProcess::nullDevice());
}

TemperatureMonitor::~TemperatureMonitor() { stopHelper(); }

void TemperatureMonitor::stopHelper()
{
    if (helper_.state() == QProcess::NotRunning) return;
    helper_.write("quit\n");
    helper_.closeWriteChannel();
    if (!helper_.waitForFinished(300)) {
        // Only terminate the child we own, never a user's monitoring application.
        helper_.kill();
        helper_.waitForFinished(500);
    }
}

TemperatureSnapshot TemperatureMonitor::unavailable(const QString& message)
{
    stopHelper();
    retryTimer_.start();
    failureMessage_ = message;
    return {{}, {}, false, message};
}

TemperatureSnapshot TemperatureMonitor::sample(const CancelCheck& cancelled)
{
    if (cancelled()) return {{}, {}, false, "Temperature collection cancelled"};
    if (retryTimer_.isValid() && retryTimer_.elapsed() < 30000)
        return {{}, {}, false, failureMessage_ + "; retrying shortly"};
    if (helper_.state() == QProcess::NotRunning) {
        if (!QFileInfo::exists(helperPath_))
            return unavailable("Temperature helper missing; run build-temperatures.ps1");
        helper_.start(helperPath_, arguments_);
        if (!helper_.waitForStarted(500)) return unavailable("Temperature helper could not start");
    }
    if (helper_.write("sample\n") != 7) return unavailable("Temperature helper disconnected");
    QElapsedTimer deadline;
    deadline.start();
    QByteArray response;
    while (deadline.elapsed() < timeoutMs_ && !cancelled()) {
        helper_.waitForReadyRead(50);
        response += helper_.readAllStandardOutput();
        if (response.size() > 64 * 1024) return unavailable("Temperature helper response too large");
        if (response.contains('\n')) {
            // Exactly one reply per request. A bad reply restarts the helper next time.
            auto snapshot = parseTemperatures(response.trimmed());
            if (!snapshot.helperAvailable) return unavailable(snapshot.message);
            retryTimer_.invalidate();
            return snapshot;
        }
        if (helper_.state() == QProcess::NotRunning) return unavailable("Temperature helper exited");
    }
    return unavailable(cancelled() ? "Temperature collection cancelled" : "Temperature helper timed out");
}
