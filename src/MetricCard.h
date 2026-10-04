#pragma once
#include <QFrame>
#include <optional>
class QLabel;
class QProgressBar;

// A shared layout keeps the CPU, RAM, and GPU readings aligned.
class MetricCard : public QFrame {
public:
    explicit MetricCard(const QString& title, const QString& detail = {}, QWidget* parent = nullptr);
    void setUsage(std::optional<double> percent);
    void setDetail(const QString& detail);
    void setTemperature(const QString& text, const QString& tooltip = {});
private:
    QLabel* value_;
    QLabel* detail_;
    QLabel* temperature_;
    QProgressBar* bar_;
};
