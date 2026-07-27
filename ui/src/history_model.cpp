#include "ui/history_model.h"

namespace ui {

HistoryModel::HistoryModel(QObject* parent)
    : QAbstractListModel(parent) {}

int HistoryModel::rowCount(const QModelIndex&) const {
    return points_.size();
}

QVariant HistoryModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= points_.size())
        return {};

    const auto& p = points_[index.row()];
    switch (role) {
    case DeviceIdRole: return p.deviceId;
    case SensorRole: return p.sensor;
    case ValueRole: return p.value;
    case UnitRole: return p.unit;
    case TimestampRole: return p.timestamp;
    case AnomalyRole: return p.anomaly;
    default: return {};
    }
}

QHash<int, QByteArray> HistoryModel::roleNames() const {
    return {
        {DeviceIdRole, "deviceId"},
        {SensorRole, "sensor"},
        {ValueRole, "value"},
        {UnitRole, "unit"},
        {TimestampRole, "timestamp"},
        {AnomalyRole, "anomaly"},
    };
}

void HistoryModel::add_point(const QString& deviceId, const QString& sensor,
                              double value, const QString& unit,
                              const QDateTime& timestamp, bool anomaly) {
    beginInsertRows({}, points_.size(), points_.size());
    points_.push_back({deviceId, sensor, value, unit, timestamp, anomaly});
    endInsertRows();
}

void HistoryModel::clear() {
    beginResetModel();
    points_.clear();
    endResetModel();
}

}  // namespace ui
