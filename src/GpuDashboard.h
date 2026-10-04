#pragma once

#include "GpuMonitor.h"
#include "TemperatureSnapshot.h"
#include <QWidget>

class QLabel;
class QProgressBar;
class QVBoxLayout;

class GpuDashboard : public QWidget {
public:
    explicit GpuDashboard(QWidget* parent = nullptr);
    void display(const GpuSnapshot& snapshot);
    void displayTemperatures(const TemperatureSnapshot& snapshot);
private:
    struct Card {
        GpuKey key;
        std::wstring name;
        QWidget* frame;
        QLabel* value;
        QLabel* detail;
        QLabel* temperature;
        QProgressBar* bar;
    };
    void rebuild(const std::vector<GpuReading>& adapters);
    void updateTemperatures();
    QVBoxLayout* layout_;
    std::vector<Card> cards_;
    TemperatureSnapshot temperatures_;
};
