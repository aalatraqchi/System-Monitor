#include "ProcessMonitor.h"
#include "ProcessTableModel.h"
#include <QCoreApplication>
#include <iostream>

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    auto require = [](bool condition, const char* message) {
        if (!condition) { std::cerr << message << '\n'; std::exit(1); }
    };
    const auto live = ProcessMonitor{}.sample();
    require(live.has_value() && !live->empty(), "Process enumeration failed");
    bool foundSelf = false;
    for (const auto& process : *live) {
        if (process.id == static_cast<std::uint32_t>(QCoreApplication::applicationPid())) {
            foundSelf = true;
            require(process.workingSetBytes && *process.workingSetBytes > 0, "Own memory unavailable");
        }
    }
    require(foundSelf, "Current process missing");

    ProcessTableModel model;
    model.setProcesses({{10, L"zeta.exe", 2 * 1024 * 1024},
                        {2, L"Alpha.exe", 100 * 1024 * 1024},
                        {7, L"protected.exe", std::nullopt},
                        {8, L"zero.exe", 0}});
    ProcessSortModel sorted;
    sorted.setSourceModel(&model);
    auto pid = [&](int row) { return sorted.index(row, ProcessTableModel::Id).data().toUInt(); };
    sorted.sort(ProcessTableModel::Memory, Qt::DescendingOrder);
    require(pid(0) == 2 && pid(1) == 10 && pid(2) == 8 && pid(3) == 7, "Descending numeric memory sort failed");
    sorted.sort(ProcessTableModel::Memory, Qt::AscendingOrder);
    require(pid(0) == 8 && pid(1) == 10 && pid(2) == 2 && pid(3) == 7, "Ascending memory sort failed");
    sorted.sort(ProcessTableModel::Id, Qt::AscendingOrder);
    require(pid(0) == 2 && pid(3) == 10, "Numeric PID sort failed");
    sorted.sort(ProcessTableModel::Name, Qt::AscendingOrder);
    require(pid(0) == 2, "Case-insensitive name sort failed");
    require(model.index(2, ProcessTableModel::Memory).data().toString() == "Unavailable", "Unavailable rendered as zero");
    model.setProcesses({{42, L"new.exe", 1234}});
    require(sorted.rowCount() == 1 && pid(0) == 42, "Exited processes were retained");
    std::cout << "Live collection and process table sorting passed\n";
}
