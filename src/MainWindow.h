#pragma once

#include <QMainWindow>
#include "SystemMonitor.h"
#include "MonitoringService.h"

class QLabel;
class GpuDashboard;
class MetricCard;
class QTabBar;
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
    void applyTheme(int index);

    void updateProcesses(const ProcessSample& processes);
    GpuDashboard* gpuDashboard_ = nullptr;
    CpuHistoryGraph* cpuHistory_ = nullptr;
    SystemMonitor monitor_;
    MonitoringService* monitoringService_ = nullptr;
    ProcessTableModel* processModel_ = nullptr;
    ProcessSortModel* processSort_ = nullptr;
    QTableView* processTable_ = nullptr;
    QLabel* processSummary_ = nullptr;
    MetricCard* cpuCard_ = nullptr;
    MetricCard* ramCard_ = nullptr;
    QTabBar* themeTabs_ = nullptr;
};
