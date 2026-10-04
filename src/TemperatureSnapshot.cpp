#include "TemperatureSnapshot.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <cmath>

TemperatureSnapshot parseTemperatures(const QByteArray& json)
{
    TemperatureSnapshot snapshot;
    snapshot.message = "Invalid temperature helper response";
    if (json.size() > 64 * 1024) return snapshot;
    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(json, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) return snapshot;
    const auto object = document.object();
    if (object.value("version").toInt() != 1 || !object.value("cpu").isArray()
        || !object.value("gpu").isArray()) return snapshot;
    auto read = [](const QJsonArray& items) {
        std::vector<TemperatureReading> readings;
        for (const auto& item : items) {
            if (!item.isObject()) continue;
            const auto data = item.toObject();
            const auto name = data.value("name").toString();
            if (name.isEmpty()) continue;
            TemperatureReading reading{name, data.value("sensor").toString(), std::nullopt,
                                       data.value("reason").toString()};
            const auto value = data.value("celsius");
            const auto number = value.toDouble(-1);
            if (value.isDouble() && std::isfinite(number) && number >= 0 && number <= 150)
                reading.celsius = number;
            else if (reading.reason.isEmpty()) reading.reason = "No valid temperature sensor reading";
            readings.push_back(std::move(reading));
        }
        return readings;
    };
    snapshot.cpu = read(object.value("cpu").toArray());
    snapshot.gpu = read(object.value("gpu").toArray());
    snapshot.helperAvailable = true;
    snapshot.message = object.value("message").toString();
    return snapshot;
}

TemperatureReading hottestCpuTemperature(const TemperatureSnapshot& snapshot)
{
    TemperatureReading result{{}, {}, std::nullopt, snapshot.helperAvailable
        ? "CPU sensor unavailable; check sensor driver and access" : snapshot.message};
    for (const auto& reading : snapshot.cpu) {
        if (reading.celsius && (!result.celsius || *reading.celsius > *result.celsius)) result = reading;
        else if (!result.celsius && !reading.reason.isEmpty()) result.reason = reading.reason;
    }
    return result;
}

TemperatureReading gpuTemperature(const TemperatureSnapshot& snapshot,
                                  const QString& hardwareName, bool uniqueAdapter)
{
    TemperatureReading result{hardwareName, {}, std::nullopt,
        snapshot.helperAvailable ? "GPU temperature sensor unavailable" : snapshot.message};
    // Never attach a temperature to the wrong card when identical GPU names repeat.
    if (!uniqueAdapter) { result.reason = "GPU identity is ambiguous for temperature matching"; return result; }
    int matches = 0;
    for (const auto& reading : snapshot.gpu) {
        if (reading.hardwareName.compare(hardwareName, Qt::CaseInsensitive) == 0) {
            result = reading;
            ++matches;
        }
    }
    if (matches > 1) {
        result.celsius.reset();
        result.reason = "Multiple sensors have the same GPU name; temperature unavailable";
    }
    return result;
}

QString temperatureText(const TemperatureReading& reading)
{
    return reading.celsius ? "Temperature: " + QString::number(*reading.celsius, 'f', 1) + " °C"
                           : "Temperature: Unavailable";
}
