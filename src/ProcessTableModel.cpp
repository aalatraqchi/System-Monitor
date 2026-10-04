#include "ProcessTableModel.h"

int ProcessTableModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(processes_.size());
}

int ProcessTableModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant ProcessTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()
        || index.column() < 0 || index.column() >= ColumnCount) return {};
    const auto& process = processes_[index.row()];
    if (role == Qt::TextAlignmentRole && index.column() != Name)
        return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
    if (role == Qt::ToolTipRole && index.column() == Memory)
        return process.workingSetBytes ? "Physical memory currently resident (working set)."
                                      : "Memory unavailable: access denied or process exited.";
    if (role != Qt::DisplayRole && role != SortRole) return {};
    switch (index.column()) {
    case Name: return QString::fromStdWString(process.name);
    case Id: return QVariant::fromValue(static_cast<qulonglong>(process.id));
    case Memory:
        if (role == SortRole)
            return process.workingSetBytes
                ? QVariant::fromValue(static_cast<qulonglong>(*process.workingSetBytes)) : QVariant{};
        return process.workingSetBytes
            ? QString::number(static_cast<double>(*process.workingSetBytes) / (1024.0 * 1024.0), 'f', 1)
            : QString("Unavailable");
    default: return {};
    }
}

QVariant ProcessTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) return {};
    switch (section) {
    case Name: return "Process";
    case Id: return "PID";
    case Memory: return "Memory (MiB)";
    default: return {};
    }
}

void ProcessTableModel::setProcesses(std::vector<ProcessInfo> processes)
{
    beginResetModel();
    processes_ = std::move(processes);
    endResetModel();
}

ProcessSortModel::ProcessSortModel(QObject* parent) : QSortFilterProxyModel(parent)
{
    setSortRole(ProcessTableModel::SortRole);
    setSortCaseSensitivity(Qt::CaseInsensitive);
}

bool ProcessSortModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
{
    const auto a = sourceModel()->data(left, sortRole());
    const auto b = sourceModel()->data(right, sortRole());
    // Keep unavailable memory at the bottom in either sort direction.
    if (a.isValid() != b.isValid())
        return sortOrder() == Qt::AscendingOrder ? a.isValid() : !a.isValid();
    if (a == b) {
        return sourceModel()->data(left.siblingAtColumn(ProcessTableModel::Id), sortRole()).toULongLong()
             < sourceModel()->data(right.siblingAtColumn(ProcessTableModel::Id), sortRole()).toULongLong();
    }
    return QSortFilterProxyModel::lessThan(left, right);
}
