#pragma once

#include <QMainWindow>
#include "SystemMonitor.h"
#include "MonitoringService.h"

class QLabel;
class GpuDashboard;
class QProgressBar;
class QTableView;
class CpuHistoryGraph;
class ProcessTableModel;
class ProcessSortModel;

class MainWindow : public QMainWindow {
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private:
    void buildInterface();
    void refreshDisplay();
    static void updateMetric(QLabel* label, QProgressBar* bar,
                             const std::optional<double>& percent);

    void updateProcesses(const ProcessSample& processes);
    GpuDashboard* gpuDashboard_ = nullptr;
    CpuHistoryGraph* cpuHistory_ = nullptr;
    SystemMonitor monitor_;
    MonitoringService* monitoringService_ = nullptr;
    ProcessTableModel* processModel_ = nullptr;
    ProcessSortModel* processSort_ = nullptr;
    QTableView* processTable_ = nullptr;
    QLabel* processSummary_ = nullptr;
    QLabel* cpuValue_ = nullptr;
    QLabel* cpuDetail_ = nullptr;
    QLabel* cpuTemperature_ = nullptr;
    QLabel* ramValue_ = nullptr;
    QLabel* ramDetail_ = nullptr;
    QProgressBar* cpuBar_ = nullptr;
    QProgressBar* ramBar_ = nullptr;
};
