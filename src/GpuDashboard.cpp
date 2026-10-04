#include "GpuDashboard.h"
#include "MetricCard.h"
#include <QVBoxLayout>
#include <algorithm>

GpuDashboard::GpuDashboard(QWidget* parent) : QWidget(parent), layout_(new QVBoxLayout(this))
{
    layout_->setContentsMargins(0, 0, 0, 0);
    layout_->setSpacing(18);
    rebuild({{{0, 0}, L"", std::nullopt, {}, GpuReadingStatus::Waiting}});
    cards_[0].widget->setDetail("Collecting GPU data…");
}

void GpuDashboard::rebuild(const std::vector<GpuReading>& adapters)
{
    for (const auto& card : cards_) delete card.widget;
    cards_.clear();
    for (std::size_t i = 0; i < adapters.size(); ++i) {
        const auto& adapter = adapters[i];
        auto* card = new MetricCard(adapter.name.empty() ? "GPU" : QString("GPU %1").arg(i));
        card->setTemperature("Temperature: Collecting…");
        layout_->addWidget(card);
        cards_.push_back({adapter.key, adapter.name, card});
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
        card.widget->setUsage(gpu.percent);
        QString detail = QString::fromStdWString(gpu.name);
        if (!detail.isEmpty()) detail += "\n";
        if (gpu.status == GpuReadingStatus::Live)
            detail += "Busiest engine: " + QString::fromStdWString(gpu.busiestEngine);
        else if (gpu.status == GpuReadingStatus::Waiting) detail += "Waiting for usage samples";
        else detail += snapshot.enumerationAvailable && gpu.name.empty()
            ? "No hardware GPU detected" : "GPU usage unavailable";
        card.widget->setDetail(detail);
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
        card.widget->setTemperature(temperatureText(reading), reading.celsius ? reading.sensorName : reading.reason);
    }
}
