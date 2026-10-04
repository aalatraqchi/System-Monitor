#include "MainWindow.h"
#include "CpuHistoryGraph.h"
#include "GpuDashboard.h"
#include <QScrollArea>
#include "ProcessTableModel.h"
#include <QTableView>
#include <QHeaderView>
#include <QScrollBar>

#include <QApplication>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QStatusBar>
#include <QTimer>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>

namespace {
// Layouts handle sizing and spacing so the window remains usable when resized.
QFrame* createMetricCard(const QString& title, const QString& description,
                        QLabel*& value, QProgressBar*& bar, QLabel** detailLabel = nullptr)
{
    auto* card = new QFrame;
    card->setObjectName("metricCard");
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(14);

    auto* heading = new QLabel(title);
    heading->setObjectName("metricTitle");
    value = new QLabel;
    value->setObjectName("metricValue");
    bar = new QProgressBar;
    bar->setRange(0, 100);
    bar->setTextVisible(false);
    bar->setFixedHeight(10);
    auto* detail = new QLabel(description);
    detail->setObjectName("muted");
    detail->setWordWrap(true);
    if (detailLabel) {
        *detailLabel = detail;
    }

    layout->addWidget(heading);
    layout->addWidget(value);
    layout->addWidget(bar);
    layout->addWidget(detail);
    return card;
}
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    setWindowTitle("System Monitor");
    resize(900, 920);
    setMinimumSize(740, 560);
    buildInterface();
    monitoringService_ = new MonitoringService(this);
    connect(monitoringService_, &MonitoringService::sampleReady,
            this, &MainWindow::updateProcesses);
    connect(monitoringService_, &MonitoringService::gpuReady, gpuDashboard_, &GpuDashboard::display);
    connect(qApp, &QCoreApplication::aboutToQuit, monitoringService_, &MonitoringService::stop);
    refreshDisplay();

    // CPU/RAM reads stay short; process scans run independently on the worker.
    // Qt stops and deletes this timer when the window is destroyed.
    auto* refreshTimer = new QTimer(this);
    refreshTimer->setInterval(1000);
    connect(refreshTimer, &QTimer::timeout, this, &MainWindow::refreshDisplay);
    refreshTimer->start();
}

MainWindow::~MainWindow()
{
    // Join the worker before Qt tears down this window's child objects.
    monitoringService_->stop();
}

void MainWindow::buildInterface()
{
    // Qt parents own their child widgets; no manual deletion is needed.
    auto* central = new QWidget;
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidget(central);
    setCentralWidget(scroll);
    auto* layout = new QVBoxLayout(central);
    layout->setContentsMargins(28, 28, 28, 20);
    layout->setSpacing(18);

    auto* title = new QLabel("System Monitor");
    title->setObjectName("pageTitle");
    auto* subtitle = new QLabel("A simple view of your Windows system.");
    subtitle->setObjectName("muted");
    layout->addWidget(title);
    layout->addWidget(subtitle);

    auto* metrics = new QHBoxLayout;
    metrics->setSpacing(18);
    metrics->addWidget(createMetricCard("CPU", "Overall processor usage",
                                       cpuValue_, cpuBar_, &cpuDetail_));
    metrics->addWidget(createMetricCard("RAM", "Physical memory usage",
                                       ramValue_, ramBar_, &ramDetail_));
    gpuDashboard_ = new GpuDashboard;
    metrics->addWidget(gpuDashboard_);
    for (int column = 0; column < 3; ++column) metrics->setStretch(column, 1);
    layout->addLayout(metrics);

    cpuHistory_ = new CpuHistoryGraph;
    layout->addWidget(cpuHistory_);

    auto* note = new QLabel("CPU, RAM, GPU, and processes refresh every second. Click a column to sort.");
    note->setObjectName("muted");
    note->setWordWrap(true);
    layout->addWidget(note);
    processSummary_ = new QLabel("Collecting process data…");
    layout->addWidget(processSummary_);
    processModel_ = new ProcessTableModel(this);
    processSort_ = new ProcessSortModel(this);
    processSort_->setSourceModel(processModel_);
    processTable_ = new QTableView;
    processTable_->setModel(processSort_);
    processTable_->setSortingEnabled(true);
    processTable_->sortByColumn(ProcessTableModel::Memory, Qt::DescendingOrder);
    processTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    processTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    processTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    processTable_->setAlternatingRowColors(true);
    processTable_->verticalHeader()->hide();
    processTable_->horizontalHeader()->setSectionResizeMode(ProcessTableModel::Name, QHeaderView::Stretch);
    processTable_->horizontalHeader()->setSectionResizeMode(ProcessTableModel::Id, QHeaderView::ResizeToContents);
    processTable_->horizontalHeader()->setSectionResizeMode(ProcessTableModel::Memory, QHeaderView::ResizeToContents);
    processTable_->setMinimumHeight(160);
    layout->addWidget(processTable_, 1);

    auto* actions = new QHBoxLayout;
    auto* refresh = new QPushButton("Refresh");
    connect(refresh, &QPushButton::clicked, this, &MainWindow::refreshDisplay);
    actions->addStretch();
    actions->addWidget(refresh);
    layout->addLayout(actions);

    setStyleSheet(R"(
        QMainWindow, QWidget { background: #111827; color: #e5e7eb;
                              font-family: 'Segoe UI'; font-size: 14px; }
        QLabel#pageTitle { font-size: 28px; font-weight: 700; }
        QLabel#muted { color: #9ca3af; }
        QFrame#metricCard { background: #1f2937; border: 1px solid #374151;
                           border-radius: 12px; }
        QFrame#metricCard QLabel { background: transparent; }
        QLabel#metricTitle { font-size: 16px; font-weight: 600; }
        QLabel#metricValue { font-size: 38px; font-weight: 600; }
        QProgressBar { background: #374151; border: none; border-radius: 5px; }
        QProgressBar::chunk { background: #38bdf8; border-radius: 5px; }
        QPushButton { background: #2563eb; border: none; border-radius: 6px;
                      padding: 10px 22px; font-weight: 600; }
        QPushButton:hover { background: #3b82f6; }
        QPushButton:pressed { background: #1d4ed8; }
        QTableView { background: #111827; alternate-background-color: #1f2937;
                     gridline-color: #374151; selection-background-color: #2563eb;
                     selection-color: #ffffff; border: 1px solid #374151; }
        QHeaderView::section { background: #1f2937; padding: 8px; border: none;
                               border-bottom: 1px solid #374151; font-weight: 600; }
        QTableView QTableCornerButton::section { background: #1f2937; }
        QStatusBar { color: #9ca3af; }
    )");
}

void MainWindow::refreshDisplay()
{
    monitoringService_->requestRefresh();
    const auto snapshot = monitor_.sample();
    updateMetric(cpuValue_, cpuBar_, snapshot.cpuPercent);
    cpuHistory_->addSample(snapshot.cpuPercent);
    cpuDetail_->setText(snapshot.cpuPercent
        ? "Overall processor usage" : "Waiting for two valid CPU samples");
    const auto percent = snapshot.memory
        ? std::optional<double>{snapshot.memory->percent} : std::nullopt;
    updateMetric(ramValue_, ramBar_, percent);

    if (!snapshot.memory) {
        ramDetail_->setText("Physical memory unavailable");
        statusBar()->showMessage("RAM unavailable — retrying every second.");
        return;
    }

    // GiB uses 1024-based units, matching the byte counts supplied by Windows.
    constexpr double bytesPerGiB = 1024.0 * 1024.0 * 1024.0;
    const auto& memory = *snapshot.memory;
    ramDetail_->setText(QString("%1 / %2 GiB used")
        .arg(static_cast<double>(memory.usedBytes) / bytesPerGiB, 0, 'f', 2)
        .arg(static_cast<double>(memory.totalBytes) / bytesPerGiB, 0, 'f', 2));
    statusBar()->showMessage(snapshot.cpuPercent
        ? "Live CPU and RAM monitoring — updates every second"
        : "Collecting CPU samples — RAM is live");
}

void MainWindow::updateMetric(QLabel* label, QProgressBar* bar,
                              const std::optional<double>& percent)
{
    if (!percent || !std::isfinite(*percent)) {
        label->setText("-- %");
        bar->setValue(0);
        bar->setToolTip("Measurement not available yet");
        return;
    }

    const auto value = std::clamp(*percent, 0.0, 100.0);
    label->setText(QString::number(value, 'f', 1) + " %");
    bar->setValue(static_cast<int>(std::lround(value)));
    bar->setToolTip(label->text());
}

// Preserve the selected PID and scroll position while replacing each snapshot.
void MainWindow::updateProcesses(const ProcessSample& processes)
{
    Q_ASSERT(QThread::currentThread() == thread());
    if (!processes) {
        processSummary_->setText("Processes unavailable — keeping the last reading and retrying");
        return;
    }

    const auto selected = processTable_->currentIndex();
    const bool hadSelection = selected.isValid();
    const auto selectedPid = hadSelection
        ? selected.siblingAtColumn(ProcessTableModel::Id).data().toULongLong() : 0;
    const int scroll = processTable_->verticalScrollBar()->value();
    int unavailable = 0;
    for (const auto& process : *processes)
        if (!process.workingSetBytes) ++unavailable;
    processModel_->setProcesses(*processes);
    if (hadSelection) {
        for (int row = 0; row < processSort_->rowCount(); ++row) {
            if (processSort_->index(row, ProcessTableModel::Id).data().toULongLong() == selectedPid) {
                processTable_->selectRow(row);
                break;
            }
        }
    }
    processTable_->verticalScrollBar()->setValue(scroll);
    processSummary_->setText(QString("Running processes: %1  •  Memory unavailable: %2")
        .arg(processModel_->rowCount()).arg(unavailable));
}
