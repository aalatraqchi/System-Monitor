#pragma once

#include <QMainWindow>
#include "SystemMonitor.h"
#include "ProcessMonitor.h"

class QLabel;
class QProgressBar;
class QTableView;
class CpuHistoryGraph;
class ProcessTableModel;
class ProcessSortModel;

class MainWindow : public QMainWindow {
public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    void buildInterface();
    void refreshDisplay();
    static void updateMetric(QLabel* label, QProgressBar* bar,
                             const std::optional<double>& percent);

    void refreshProcesses();
    CpuHistoryGraph* cpuHistory_ = nullptr;
    SystemMonitor monitor_;
    ProcessMonitor processMonitor_;
    ProcessTableModel* processModel_ = nullptr;
    ProcessSortModel* processSort_ = nullptr;
    QTableView* processTable_ = nullptr;
    QLabel* processSummary_ = nullptr;
    QLabel* cpuValue_ = nullptr;
    QLabel* cpuDetail_ = nullptr;
    QLabel* ramValue_ = nullptr;
    QLabel* ramDetail_ = nullptr;
    QProgressBar* cpuBar_ = nullptr;
    QProgressBar* ramBar_ = nullptr;
};
