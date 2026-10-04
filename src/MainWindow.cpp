#include "MainWindow.h"

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
    resize(780, 480);
    setMinimumSize(560, 380);
    buildInterface();
    refreshDisplay();

    // Schedule short samples without blocking the GUI event loop.
    // Qt stops and deletes this timer when the window is destroyed.
    auto* refreshTimer = new QTimer(this);
    refreshTimer->setInterval(1000);
    connect(refreshTimer, &QTimer::timeout, this, &MainWindow::refreshDisplay);
    refreshTimer->start();
}

void MainWindow::buildInterface()
{
    // Qt parents own their child widgets; no manual deletion is needed.
    auto* central = new QWidget(this);
    setCentralWidget(central);
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
                                       cpuValue_, cpuBar_));
    metrics->addWidget(createMetricCard("RAM", "Physical memory usage",
                                       ramValue_, ramBar_, &ramDetail_));
    layout->addLayout(metrics);

    auto* note = new QLabel("RAM refreshes every second. CPU monitoring is coming next.");
    note->setObjectName("muted");
    note->setWordWrap(true);
    layout->addWidget(note);
    layout->addStretch();

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
        QStatusBar { color: #9ca3af; }
    )");
}

void MainWindow::refreshDisplay()
{
    const auto snapshot = monitor_.sample();
    updateMetric(cpuValue_, cpuBar_, snapshot.cpuPercent);
    const auto percent = snapshot.memory
        ? std::optional<double>{snapshot.memory->percent} : std::nullopt;
    updateMetric(ramValue_, ramBar_, percent);

    if (!snapshot.memory) {
        ramDetail_->setText("Physical memory unavailable");
        statusBar()->showMessage("RAM could not be read. Retrying every second.");
        return;
    }

    // GiB uses 1024-based units, matching the byte counts supplied by Windows.
    constexpr double bytesPerGiB = 1024.0 * 1024.0 * 1024.0;
    const auto& memory = *snapshot.memory;
    ramDetail_->setText(QString("%1 / %2 GiB used")
        .arg(static_cast<double>(memory.usedBytes) / bytesPerGiB, 0, 'f', 2)
        .arg(static_cast<double>(memory.totalBytes) / bytesPerGiB, 0, 'f', 2));
    statusBar()->showMessage("Live RAM monitoring — updates every second");
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
