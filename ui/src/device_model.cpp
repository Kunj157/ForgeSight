#include "ui/device_model.h"

#include <QVariantMap>

#include <algorithm>

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
    case PlantRole:
        return d.plant;
    case FloorRole:
        return d.floor;
    default:
        return {};
    }
}

QHash<int, QByteArray> DeviceModel::roleNames() const {
    return {
        {DeviceIdRole, "deviceId"}, {SensorRole, "sensor"},       {ValueRole, "value"},
        {UnitRole, "unit"},         {TimestampRole, "timestamp"}, {AnomalyRole, "anomaly"},
        {StatusRole, "status"},     {PlantRole, "plant"},         {FloorRole, "floor"},
    };
}

void DeviceModel::updateDevice(const QString& device_id, const QString& sensor, double value,
                               const QString& unit, const QString& timestamp, bool anomaly) {
    int row = find_device(device_id, sensor);
    QString status = compute_status(anomaly, value);

    QString plant;
    QString floor;

    if (row >= 0) {
        auto& d = devices_[row];
        d.value = value;
        d.unit = unit;
        d.timestamp = timestamp;
        d.anomaly = anomaly;
        d.status = status;
        plant = d.plant;
        floor = d.floor;
        auto idx = index(row);
        Q_EMIT dataChanged(idx, idx);
    } else {
        DeviceState state{device_id,
                          sensor,
                          value,
                          unit,
                          timestamp,
                          anomaly,
                          status,
                          QStringLiteral("Unassigned"),
                          QStringLiteral("Unassigned")};
        auto it = meta_.constFind(device_id);
        if (it != meta_.constEnd()) {
            state.plant = it->first;
            state.floor = it->second;
        }
        plant = state.plant;
        floor = state.floor;
        beginInsertRows(QModelIndex(), devices_.size(), devices_.size());
        devices_.append(std::move(state));
        endInsertRows();
        Q_EMIT countChanged();
    }

    Q_EMIT deviceUpdated(device_id, sensor, value, unit, timestamp, anomaly, plant, floor);
}

void DeviceModel::updateDeviceMeta(const QString& device_id, const QString& plant,
                                   const QString& floor) {
    meta_[device_id] = {plant, floor};

    int first = -1, last = -1;
    for (int i = 0; i < devices_.size(); ++i) {
        if (devices_[i].device_id != device_id)
            continue;
        devices_[i].plant = plant;
        devices_[i].floor = floor;
        if (first < 0)
            first = i;
        last = i;
    }
    if (first >= 0) {
        Q_EMIT dataChanged(index(first), index(last));
    }
}

QStringList DeviceModel::plants() const {
    QStringList result;
    for (const auto& d : devices_) {
        if (!result.contains(d.plant))
            result.append(d.plant);
    }
    std::sort(result.begin(), result.end());
    return result;
}

QStringList DeviceModel::floors(const QString& plant) const {
    QStringList result;
    for (const auto& d : devices_) {
        if (d.plant == plant && !result.contains(d.floor))
            result.append(d.floor);
    }
    std::sort(result.begin(), result.end());
    return result;
}

QVariantList DeviceModel::devicesFor(const QString& plant, const QString& floor) const {
    QVariantList result;
    for (const auto& d : devices_) {
        if (d.plant == plant && d.floor == floor)
            result.append(row_to_map(d));
    }
    return result;
}

QVariantMap DeviceModel::row_to_map(const DeviceState& d) const {
    return QVariantMap{
        {"deviceId", d.device_id}, {"sensor", d.sensor},       {"value", d.value},
        {"unit", d.unit},          {"timestamp", d.timestamp}, {"anomaly", d.anomaly},
        {"status", d.status},      {"plant", d.plant},         {"floor", d.floor},
    };
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
