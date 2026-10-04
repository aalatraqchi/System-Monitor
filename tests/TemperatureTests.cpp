#include "TemperatureMonitor.h"
#include <QCoreApplication>
#include <QElapsedTimer>
#include <cstdlib>
#include <iostream>

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    auto require = [](bool value, const char* message) {
        if (!value) { std::cerr << message << '\n'; std::exit(1); }
    };
    const auto snapshot = parseTemperatures(R"({"version":1,"cpu":[
        {"name":"CPU 0","sensor":"CPU Package","celsius":52},
        {"name":"CPU 1","sensor":"CPU Package","celsius":65}],"gpu":[
        {"name":"GPU A","sensor":"GPU Core","celsius":0},
        {"name":"GPU B","sensor":"GPU Core","celsius":49.5}]})");
    require(snapshot.helperAvailable && hottestCpuTemperature(snapshot).celsius == 65,
            "Hottest CPU selection failed");
    require(gpuTemperature(snapshot, "gpu b", true).celsius == 49.5, "GPU matching failed");
    require(gpuTemperature(snapshot, "GPU A", true).celsius == 0, "Valid zero became unavailable");
    require(!gpuTemperature(snapshot, "GPU A", false).celsius, "Ambiguous adapter was matched");
    require(!gpuTemperature(snapshot, "Unknown GPU", true).celsius, "Unknown GPU was matched");
    const auto missing = parseTemperatures(R"({"version":1,"cpu":[
        {"name":"CPU","celsius":null,"reason":"Driver missing"}],"gpu":[]})");
    require(!hottestCpuTemperature(missing).celsius && hottestCpuTemperature(missing).reason == "Driver missing",
            "Missing sensor lost its reason");
    const auto invalid = parseTemperatures(R"({"version":1,"cpu":[
        {"name":"CPU","celsius":999}],"gpu":[{"name":"GPU","celsius":"50"}]})");
    require(!hottestCpuTemperature(invalid).celsius && !gpuTemperature(invalid, "GPU", true).celsius,
            "Invalid temperature accepted");
    const auto duplicates = parseTemperatures(R"({"version":1,"cpu":[],"gpu":[
        {"name":"GPU","celsius":40},{"name":"GPU","celsius":80}]})");
    require(!gpuTemperature(duplicates, "GPU", true).celsius, "Duplicate helper identity was matched");
    require(!parseTemperatures("not JSON").helperAvailable
            && !parseTemperatures(R"({"version":2,"cpu":[],"gpu":[]})").helperAvailable,
            "Invalid protocol accepted");
    require(temperatureText(hottestCpuTemperature(missing)).contains("Unavailable"), "Missing reading displayed as zero");

    const QString fixture = QCoreApplication::applicationDirPath() + "/TemperatureHelperFixture.exe";
    const auto noCancel = [] { return false; };
    TemperatureMonitor good(fixture, {"--stdio", "valid"}, 1000);
    require(hottestCpuTemperature(good.sample(noCancel)).celsius == 52, "Helper reply not read");
    require(hottestCpuTemperature(good.sample(noCancel)).celsius == 52, "Persistent helper failed on second request");
    TemperatureMonitor malformed(fixture, {"--stdio", "invalid"}, 1000);
    require(!malformed.sample(noCancel).helperAvailable, "Malformed helper reply accepted");
    TemperatureMonitor exited(fixture, {"--stdio", "exit"}, 1000);
    require(!exited.sample(noCancel).helperAvailable, "Exited helper reported live data");
    TemperatureMonitor absent(fixture + ".missing");
    require(!absent.sample(noCancel).helperAvailable, "Missing helper reported live data");
    QElapsedTimer elapsed;
    elapsed.start();
    TemperatureMonitor hung(fixture, {"--stdio", "hang"}, 150);
    require(!hung.sample(noCancel).helperAvailable && elapsed.elapsed() < 1500, "Hung helper was not bounded");
    elapsed.restart();
    require(!hung.sample(noCancel).helperAvailable && elapsed.elapsed() < 100, "Failed helper retried too quickly");
    TemperatureMonitor cancelled(fixture, {"--stdio", "hang"}, 2500);
    elapsed.restart();
    require(!cancelled.sample([&] { return elapsed.elapsed() > 100; }).helperAvailable
            && elapsed.elapsed() < 1500, "Cancellation did not stop helper wait");
    std::cout << "Sensor parsing, identity matching, missing values, helper lifecycle, timeout and cancellation passed\n";
}
