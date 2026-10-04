#pragma once

#include <QByteArray>
#include <QString>
#include <optional>
#include <vector>

struct TemperatureReading {
    QString hardwareName;
    QString sensorName;
    std::optional<double> celsius;
    QString reason;
};
struct TemperatureSnapshot {
    std::vector<TemperatureReading> cpu;
    std::vector<TemperatureReading> gpu;
    bool helperAvailable = false;
    QString message;
};

// Validate the helper boundary before any numbers reach the dashboard.
TemperatureSnapshot parseTemperatures(const QByteArray& json);
TemperatureReading hottestCpuTemperature(const TemperatureSnapshot& snapshot);
TemperatureReading gpuTemperature(const TemperatureSnapshot& snapshot,
                                  const QString& hardwareName, bool uniqueAdapter);
QString temperatureText(const TemperatureReading& reading);
