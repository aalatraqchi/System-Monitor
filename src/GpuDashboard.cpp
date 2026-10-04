#include "GpuDashboard.h"
#include <QFrame>
#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>
#include <cmath>
#include <algorithm>

GpuDashboard::GpuDashboard(QWidget* parent) : QWidget(parent), layout_(new QVBoxLayout(this))
{
    layout_->setContentsMargins(0, 0, 0, 0);
    layout_->setSpacing(18);
    rebuild({{{0, 0}, L"", std::nullopt, {}, GpuReadingStatus::Waiting}});
    cards_[0].detail->setText("Collecting GPU data…");
}

void GpuDashboard::rebuild(const std::vector<GpuReading>& adapters)
{
    for (const auto& card : cards_) delete card.frame;
    cards_.clear();
    for (std::size_t i = 0; i < adapters.size(); ++i) {
        const auto& adapter = adapters[i];
        auto* frame = new QFrame;
        frame->setObjectName("metricCard");
        auto* contents = new QVBoxLayout(frame);
        contents->setContentsMargins(24, 24, 24, 24);
        contents->setSpacing(14);
        auto* heading = new QLabel(adapter.name.empty() ? "GPU" : QString("GPU %1").arg(i));
        heading->setObjectName("metricTitle");
        auto* value = new QLabel("-- %");
        value->setObjectName("metricValue");
        auto* bar = new QProgressBar;
        bar->setRange(0, 100);
        bar->setTextVisible(false);
        bar->setFixedHeight(10);
        auto* detail = new QLabel;
        detail->setObjectName("muted");
        detail->setWordWrap(true);
        auto* temperature = new QLabel("Temperature: Collecting…");
        temperature->setObjectName("muted");
        temperature->setWordWrap(true);
        contents->addWidget(heading);
        contents->addWidget(value);
        contents->addWidget(bar);
        contents->addWidget(detail);
        contents->addWidget(temperature);
        layout_->addWidget(frame);
        cards_.push_back({adapter.key, adapter.name, frame, value, detail, temperature, bar});
    }
}

void GpuDashboard::display(const GpuSnapshot& snapshot)
{
    auto adapters = snapshot.adapters;
    if (!snapshot.enumerationAvailable) {
        // Keep known hardware labels through a temporary enumeration failure.
        adapters.clear();
        for (const auto& card : cards_)
            adapters.push_back({card.key, card.name, std::nullopt, {}, GpuReadingStatus::Unavailable});
    }
    if (adapters.empty()) adapters.push_back({{0, 0}, L"", std::nullopt, {}, GpuReadingStatus::Unavailable});
    bool changed = adapters.size() != cards_.size();
    for (std::size_t i = 0; !changed && i < adapters.size(); ++i)
        changed = adapters[i].key.adapterId != cards_[i].key.adapterId
               || adapters[i].key.physicalIndex != cards_[i].key.physicalIndex
               || adapters[i].name != cards_[i].name;
    if (changed) rebuild(adapters);

    for (std::size_t i = 0; i < adapters.size(); ++i) {
        const auto& gpu = adapters[i];
        auto& card = cards_[i];
        card.value->setText(gpu.percent ? QString::number(*gpu.percent, 'f', 1) + " %" : "-- %");
        card.bar->setValue(gpu.percent ? static_cast<int>(std::lround(*gpu.percent)) : 0);
        QString detail = QString::fromStdWString(gpu.name);
        if (!detail.isEmpty()) detail += "\n";
        if (gpu.status == GpuReadingStatus::Live)
            detail += "Busiest engine: " + QString::fromStdWString(gpu.busiestEngine);
        else if (gpu.status == GpuReadingStatus::Waiting) detail += "Waiting for usage samples";
        else detail += snapshot.enumerationAvailable && gpu.name.empty()
            ? "No hardware GPU detected" : "GPU usage unavailable";
        card.detail->setText(detail);
        card.bar->setToolTip(gpu.percent
            ? "Busiest GPU engine across all processes" : "No valid GPU utilization reading");
    }
    updateTemperatures();
}

void GpuDashboard::displayTemperatures(const TemperatureSnapshot& snapshot)
{
    temperatures_ = snapshot;
    updateTemperatures();
}

void GpuDashboard::updateTemperatures()
{
    for (const auto& card : cards_) {
        const auto matches = std::count_if(cards_.begin(), cards_.end(), [&](const auto& candidate) {
            return QString::fromStdWString(candidate.name).compare(
                QString::fromStdWString(card.name), Qt::CaseInsensitive) == 0;
        });
        const auto reading = gpuTemperature(temperatures_, QString::fromStdWString(card.name),
                                            matches == 1 && card.key.physicalIndex == 0);
        card.temperature->setText(temperatureText(reading));
        card.temperature->setToolTip(reading.celsius ? reading.sensorName : reading.reason);
    }
}
