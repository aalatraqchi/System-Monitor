#pragma once

#include "CpuHistory.h"
#include <QElapsedTimer>
#include <QWidget>

class CpuHistoryGraph : public QWidget {
public:
    explicit CpuHistoryGraph(QWidget* parent = nullptr);
    void addSample(std::optional<double> percent);
protected:
    void paintEvent(QPaintEvent* event) override;
private:
    CpuHistory history_;
    QElapsedTimer clock_;
    qint64 nowMs_ = 0;
};
