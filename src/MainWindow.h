#pragma once

#include <QMainWindow>
#include "SystemMonitor.h"

class QLabel;
class QProgressBar;

class MainWindow : public QMainWindow {
public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    void buildInterface();
    void refreshDisplay();
    static void updateMetric(QLabel* label, QProgressBar* bar,
                             const std::optional<double>& percent);

    SystemMonitor monitor_;
    QLabel* cpuValue_ = nullptr;
    QLabel* cpuDetail_ = nullptr;
    QLabel* ramValue_ = nullptr;
    QLabel* ramDetail_ = nullptr;
    QProgressBar* cpuBar_ = nullptr;
    QProgressBar* ramBar_ = nullptr;
};
