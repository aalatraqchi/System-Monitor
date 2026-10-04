#include "MetricCard.h"
#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>

MetricCard::MetricCard(const QString& title, const QString& detail, QWidget* parent) : QFrame(parent)
{
    setObjectName("metricCard");
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(22, 20, 22, 20);
    layout->setSpacing(12);
    auto* heading = new QLabel(title);
    heading->setObjectName("metricTitle");
    value_ = new QLabel("-- %");
    value_->setObjectName("metricValue");
    bar_ = new QProgressBar;
    bar_->setRange(0, 100);
    bar_->setTextVisible(false);
    bar_->setFixedHeight(8);
    detail_ = new QLabel(detail);
    detail_->setObjectName("muted");
    detail_->setWordWrap(true);
    detail_->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    detail_->setMinimumHeight(40);
    temperature_ = new QLabel(" ");
    temperature_->setObjectName("muted");
    temperature_->setWordWrap(true);
    temperature_->setMinimumHeight(20);
    layout->addWidget(heading);
    layout->addWidget(value_);
    layout->addWidget(bar_);
    layout->addWidget(detail_);
    layout->addStretch();
    layout->addWidget(temperature_);
    setMinimumHeight(234);
    setUsage(std::nullopt);
}

void MetricCard::setUsage(std::optional<double> percent)
{
    const bool valid = percent && std::isfinite(*percent);
    const auto value = valid ? std::clamp(*percent, 0.0, 100.0) : 0;
    value_->setText(valid ? QString::number(value, 'f', 1) + " %" : "-- %");
    bar_->setValue(static_cast<int>(std::lround(value)));
    bar_->setToolTip(valid ? value_->text() : "Usage reading unavailable");
}
void MetricCard::setDetail(const QString& detail) { detail_->setText(detail); }
void MetricCard::setTemperature(const QString& text, const QString& tooltip)
{
    temperature_->setText(text);
    temperature_->setToolTip(tooltip);
}
