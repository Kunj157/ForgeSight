#include "ui/device_model.h"

namespace ui {

DeviceModel::DeviceModel(QObject* parent) : QAbstractListModel(parent) {}

int DeviceModel::rowCount(const QModelIndex&) const {
    return devices_.size();
}

QVariant DeviceModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= devices_.size())
        return {};

    const auto& d = devices_[index.row()];
    switch (role) {
    case DeviceIdRole:
        return d.device_id;
    case SensorRole:
        return d.sensor;
    case ValueRole:
        return d.value;
    case UnitRole:
        return d.unit;
    case TimestampRole:
        return d.timestamp;
    case AnomalyRole:
        return d.anomaly;
    case StatusRole:
        return d.status;
    default:
        return {};
    }
}

QHash<int, QByteArray> DeviceModel::roleNames() const {
    return {
        {DeviceIdRole, "deviceId"}, {SensorRole, "sensor"},       {ValueRole, "value"},
        {UnitRole, "unit"},         {TimestampRole, "timestamp"}, {AnomalyRole, "anomaly"},
        {StatusRole, "status"},
    };
}

void DeviceModel::updateDevice(const QString& device_id, const QString& sensor, double value,
                               const QString& unit, const QString& timestamp, bool anomaly) {
    int row = find_device(device_id, sensor);
    QString status = compute_status(anomaly, value);

    if (row >= 0) {
        auto& d = devices_[row];
        d.value = value;
        d.unit = unit;
        d.timestamp = timestamp;
        d.anomaly = anomaly;
        d.status = status;
        auto idx = index(row);
        Q_EMIT dataChanged(idx, idx);
    } else {
        beginInsertRows(QModelIndex(), devices_.size(), devices_.size());
        devices_.append({device_id, sensor, value, unit, timestamp, anomaly, status});
        endInsertRows();
        Q_EMIT countChanged();
    }
}

void DeviceModel::clear() {
    if (devices_.isEmpty())
        return;
    beginResetModel();
    devices_.clear();
    endResetModel();
    Q_EMIT countChanged();
}

QString DeviceModel::device_status(const QString& device_id) const {
    for (int i = 0; i < devices_.size(); ++i) {
        if (devices_[i].device_id == device_id)
            return devices_[i].status;
    }
    return {};
}

int DeviceModel::find_device(const QString& device_id, const QString& sensor) const {
    for (int i = 0; i < devices_.size(); ++i) {
        if (devices_[i].device_id == device_id && devices_[i].sensor == sensor)
            return i;
    }
    return -1;
}

QString DeviceModel::compute_status(bool anomaly, double /*value*/) {
    if (anomaly)
        return "critical";
    return "normal";
}

} // namespace ui
