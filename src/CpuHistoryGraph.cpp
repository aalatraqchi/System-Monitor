#include "CpuHistoryGraph.h"

#include <QApplication>
#include <QPainter>
#include <QPainterPath>

CpuHistoryGraph::CpuHistoryGraph(QWidget* parent) : QWidget(parent)
{
    clock_.start();
    setObjectName("cpuHistory");
    setMinimumHeight(170);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setAccessibleName("CPU usage history, last 60 seconds");
    setToolTip("CPU usage over the last 60 seconds. Missing readings appear as gaps.");
}

void CpuHistoryGraph::addSample(std::optional<double> percent)
{
    nowMs_ = clock_.elapsed();
    history_.record(nowMs_, percent);
    setAccessibleDescription(percent
        ? QString("Current CPU usage: %1 percent").arg(*percent, 0, 'f', 1)
        : QString("CPU reading unavailable"));
    update();
}

void CpuHistoryGraph::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    // Use the application palette so widget styles cannot dilute axis colors.
    const auto colors = QApplication::palette();
    painter.setPen(QPen(colors.color(QPalette::Mid), 1));
    painter.setBrush(colors.color(QPalette::Base));
    painter.drawRoundedRect(QRectF(rect()).adjusted(1, 1, -1, -1), 10, 10);
    painter.setPen(colors.color(QPalette::Text));
    painter.drawText(QRectF(18, 8, width() - 36, 26), "CPU history — last 60 seconds");

    const QRectF plot(52, 44, width() - 72, height() - 72);
    for (int value : {0, 50, 100}) {
        const double y = plot.bottom() - plot.height() * value / 100.0;
        painter.setPen(QPen(colors.color(QPalette::Mid), 1));
        painter.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));
        painter.setPen(colors.color(QPalette::PlaceholderText));
        painter.drawText(QRectF(6, y - 9, 40, 18), Qt::AlignRight | Qt::AlignVCenter,
                         QString::number(value) + "%");
    }
    painter.drawText(QRectF(plot.left(), plot.bottom() + 5, 100, 18), "60 seconds ago");
    painter.drawText(QRectF(plot.right() - 45, plot.bottom() + 5, 45, 18), Qt::AlignRight, "Now");

    QPainterPath line;
    bool connected = false;
    bool hasReading = false;
    std::int64_t previousTime = 0;
    painter.setClipRect(plot.adjusted(-3, -3, 3, 3));
    painter.setPen(QPen(colors.color(QPalette::Link), 2));
    painter.setBrush(colors.color(QPalette::Link));
    for (const auto& point : history_.points()) {
        if (!point.percent) { connected = false; continue; }
        const double age = static_cast<double>(nowMs_ - point.timeMs);
        const QPointF position(plot.right() - plot.width() * age / CpuHistory::windowMs,
                               plot.bottom() - plot.height() * *point.percent / 100.0);
        // Do not draw across failed readings or long pauses such as sleep/resume.
        if (!connected || point.timeMs - previousTime > 2500) line.moveTo(position);
        else line.lineTo(position);
        painter.drawEllipse(position, 2, 2);
        previousTime = point.timeMs;
        connected = hasReading = true;
    }
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(line);
    if (!hasReading) {
        painter.setPen(colors.color(QPalette::PlaceholderText));
        painter.drawText(plot, Qt::AlignCenter, "Waiting for CPU samples…");
    }
}
