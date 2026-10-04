#pragma once

#include "ProcessMonitor.h"
#include <QAbstractTableModel>
#include <QSortFilterProxyModel>

class ProcessTableModel : public QAbstractTableModel {
public:
    enum Column { Name, Id, Memory, ColumnCount };
    enum { SortRole = Qt::UserRole + 1 };
    explicit ProcessTableModel(QObject* parent = nullptr) : QAbstractTableModel(parent) {}
    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    void setProcesses(std::vector<ProcessInfo> processes);
private:
    std::vector<ProcessInfo> processes_;
};

class ProcessSortModel : public QSortFilterProxyModel {
public:
    explicit ProcessSortModel(QObject* parent = nullptr);
protected:
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;
};
