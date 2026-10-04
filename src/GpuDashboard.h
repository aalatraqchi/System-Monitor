#pragma once

#include "GpuMonitor.h"
#include <QWidget>

class QLabel;
class QProgressBar;
class QVBoxLayout;

class GpuDashboard : public QWidget {
public:
    explicit GpuDashboard(QWidget* parent = nullptr);
    void display(const GpuSnapshot& snapshot);
private:
    struct Card {
        GpuKey key;
        std::wstring name;
        QWidget* frame;
        QLabel* value;
        QLabel* detail;
        QProgressBar* bar;
    };
    void rebuild(const std::vector<GpuReading>& adapters);
    QVBoxLayout* layout_;
    std::vector<Card> cards_;
};
