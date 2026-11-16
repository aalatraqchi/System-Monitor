#include "MainWindow.h"
#include "AppTheme.h"
#include "CpuHistoryGraph.h"
#include "GpuDashboard.h"
#include "MetricCard.h"
#include "ProcessTableModel.h"
#include "WindowsAccess.h"
#include <QApplication>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSettings>
#include <QStatusBar>
#include <QTabBar>
#include <QTableView>
#include <QTimer>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    setWindowTitle("System Monitor");
    resize(960, 900);
    setMinimumSize(740, 560);
    buildInterface();
    monitoringService_ = new MonitoringService(this);
    connect(monitoringService_, &MonitoringService::sampleReady, this, &MainWindow::updateProcesses);
    connect(monitoringService_, &MonitoringService::gpuReady, gpuDashboard_, &GpuDashboard::display);
    connect(monitoringService_, &MonitoringService::temperaturesReady, this,
        [this](const TemperatureSnapshot& snapshot) {
            const auto reading = hottestCpuTemperature(snapshot);
            const auto sensor = reading.sensorName.contains("Package", Qt::CaseInsensitive) ? "CPU package"
                : reading.sensorName.contains("Tctl/Tdie", Qt::CaseInsensitive) ? "CPU Tctl/Tdie" : "Hottest CPU core";
            cpuCard_->setTemperature(reading.celsius
                ? QString("%1: %2 °C").arg(sensor).arg(*reading.celsius, 0, 'f', 1)
                : "Temperature: Unavailable", reading.celsius
                    ? reading.hardwareName + " / " + reading.sensorName : reading.reason);
            gpuDashboard_->displayTemperatures(snapshot);
        });
    connect(qApp, &QCoreApplication::aboutToQuit, monitoringService_, &MonitoringService::stop);
    refreshDisplay();
    // Qt owns the timer. All slow collectors remain on the worker.
    auto* refreshTimer = new QTimer(this);
    refreshTimer->setInterval(1000);
    connect(refreshTimer, &QTimer::timeout, this, &MainWindow::refreshDisplay);
    refreshTimer->start();
}

MainWindow::~MainWindow() { monitoringService_->stop(); }

void MainWindow::applyTheme(int index)
{
    const auto& theme = appThemes().at(static_cast<std::size_t>(index));
    qApp->setPalette(theme.palette());
    qApp->setStyleSheet(theme.styleSheet());
    cpuHistory_->update();
}

void MainWindow::buildInterface()
{
    // The header stays visible while the dashboard scrolls in smaller windows.
    auto* central = new QWidget;
    setCentralWidget(central);
    auto* outer = new QVBoxLayout(central);
    outer->setContentsMargins(24, 22, 24, 12);
    outer->setSpacing(14);
    auto* title = new QLabel("System Monitor");
    title->setObjectName("pageTitle");
    outer->addWidget(title);
    auto* subtitle = new QLabel(hasAdministratorAccess()
        ? "Administrator access • live usage and supported temperatures"
        : "CPU temperatures need administrator access. Run System Monitor as administrator to enable them.");
    subtitle->setObjectName("muted");
    subtitle->setWordWrap(true);
    outer->addWidget(subtitle);

    auto* themes = new QHBoxLayout;
    auto* themeLabel = new QLabel("Theme");
    themeLabel->setObjectName("muted");
    themes->addWidget(themeLabel);
    themeTabs_ = new QTabBar;
    themeTabs_->setObjectName("themeTabs");
    themeTabs_->setAccessibleName("Color theme");
    themeTabs_->setExpanding(false);
    themeTabs_->setDrawBase(false);
    themeTabs_->setFocusPolicy(Qt::StrongFocus);
    for (const auto& theme : appThemes()) {
        themeTabs_->addTab(theme.name);
        themeTabs_->setTabToolTip(themeTabs_->count() - 1, "Use the " + theme.name + " theme");
    }
    themeLabel->setBuddy(themeTabs_);
    themes->addWidget(themeTabs_);
    themes->addStretch();
    outer->addLayout(themes);

    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto* content = new QWidget;
    scroll->setWidget(content);
    outer->addWidget(scroll, 1);
    auto* layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 0, 8, 0);
    layout->setSpacing(16);
    auto* metrics = new QHBoxLayout;
    metrics->setSpacing(16);
    cpuCard_ = new MetricCard("CPU", "Overall processor usage");
    cpuCard_->setTemperature("Temperature: Collecting…");
    ramCard_ = new MetricCard("RAM", "Physical memory usage");
    gpuDashboard_ = new GpuDashboard;
    metrics->addWidget(cpuCard_, 1, Qt::AlignTop);
    metrics->addWidget(ramCard_, 1, Qt::AlignTop);
    metrics->addWidget(gpuDashboard_, 1, Qt::AlignTop);
    layout->addLayout(metrics);
    cpuHistory_ = new CpuHistoryGraph;
    layout->addWidget(cpuHistory_);

    auto* processHeading = new QHBoxLayout;
    auto* heading = new QLabel("Processes");
    heading->setObjectName("sectionTitle");
    processHeading->addWidget(heading);
    processHeading->addStretch();
    processSummary_ = new QLabel("Collecting…");
    processSummary_->setObjectName("muted");
    processHeading->addWidget(processSummary_);
    layout->addLayout(processHeading);
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
    processTable_->setShowGrid(false);
    processTable_->verticalHeader()->hide();
    processTable_->verticalHeader()->setDefaultSectionSize(30);
    processTable_->horizontalHeader()->setSectionResizeMode(ProcessTableModel::Name, QHeaderView::Stretch);
    processTable_->horizontalHeader()->setSectionResizeMode(ProcessTableModel::Id, QHeaderView::ResizeToContents);
    processTable_->horizontalHeader()->setSectionResizeMode(ProcessTableModel::Memory, QHeaderView::ResizeToContents);
    processTable_->setMinimumHeight(180);
    processTable_->setToolTip("Click a column heading to sort. Memory is the process working set.");
    layout->addWidget(processTable_, 1);

    auto* actions = new QHBoxLayout;
    auto* note = new QLabel("Updates every second • click a process heading to sort");
    note->setObjectName("muted");
    note->setWordWrap(true);
    actions->addWidget(note, 1);
    auto* refresh = new QPushButton("Refresh");
    refresh->setToolTip("Request fresh usage, process, and temperature readings");
    connect(refresh, &QPushButton::clicked, this, &MainWindow::refreshDisplay);
    actions->addWidget(refresh);
    outer->addLayout(actions);

    const int savedTheme = themeIndex(QSettings{}.value("appearance/theme", "blue").toString());
    themeTabs_->setCurrentIndex(savedTheme);
    applyTheme(savedTheme);
    connect(themeTabs_, &QTabBar::currentChanged, this, [this](int index) {
        applyTheme(index);
        QSettings{}.setValue("appearance/theme", appThemes().at(static_cast<std::size_t>(index)).id);
    });
}

void MainWindow::refreshDisplay()
{
    monitoringService_->requestRefresh();
    const auto snapshot = monitor_.sample();
    cpuCard_->setUsage(snapshot.cpuPercent);
    cpuHistory_->addSample(snapshot.cpuPercent);
    cpuCard_->setDetail(snapshot.cpuPercent ? "Overall processor usage" : "Waiting for two valid CPU samples");
    ramCard_->setUsage(snapshot.memory ? std::optional<double>{snapshot.memory->percent} : std::nullopt);
    if (!snapshot.memory) {
        ramCard_->setDetail("Physical memory unavailable");
        statusBar()->showMessage("RAM unavailable — retrying every second");
        return;
    }
    constexpr double bytesPerGiB = 1024.0 * 1024.0 * 1024.0;
    const auto& memory = *snapshot.memory;
    ramCard_->setDetail(QString("%1 / %2 GiB used")
        .arg(static_cast<double>(memory.usedBytes) / bytesPerGiB, 0, 'f', 2)
        .arg(static_cast<double>(memory.totalBytes) / bytesPerGiB, 0, 'f', 2));
    statusBar()->showMessage(snapshot.cpuPercent ? "Live monitoring • 1-second refresh"
                                              : "Collecting CPU samples • RAM is live");
}

void MainWindow::updateProcesses(const ProcessSample& processes)
{
    Q_ASSERT(QThread::currentThread() == thread());
    if (!processes) {
        processSummary_->setText("Unavailable • keeping the last reading");
        return;
    }
    // Preserve the selection and scroll position across snapshot replacements.
    const auto selected = processTable_->currentIndex();
    const bool hadSelection = selected.isValid();
    const auto selectedPid = hadSelection
        ? selected.siblingAtColumn(ProcessTableModel::Id).data().toULongLong() : 0;
    const int scroll = processTable_->verticalScrollBar()->value();
    int unavailable = 0;
    for (const auto& process : *processes) if (!process.workingSetBytes) ++unavailable;
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
    processSummary_->setText(QString("%1 running • %2 memory readings unavailable")
        .arg(processModel_->rowCount()).arg(unavailable));
}
