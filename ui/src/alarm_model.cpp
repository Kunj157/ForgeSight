#include "ui/alarm_model.h"

namespace ui {

AlarmModel::AlarmModel(QObject* parent)
    : QAbstractListModel(parent) {}

int AlarmModel::rowCount(const QModelIndex&) const {
    return alarms_.size();
}

QVariant AlarmModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= alarms_.size())
        return {};

    const auto& a = alarms_[index.row()];
    switch (role) {
        case IdRole: return a.id;
        case DeviceIdRole: return a.device_id;
        case SensorRole: return a.sensor;
        case ValueRole: return a.value;
        case SeverityRole: return a.severity;
        case MessageRole: return a.message;
        case TimestampRole: return a.timestamp;
        case AcknowledgedRole: return a.acknowledged;
        default: return {};
    }
}

QHash<int, QByteArray> AlarmModel::roleNames() const {
    return {
        {IdRole, "id"},
        {DeviceIdRole, "deviceId"},
        {SensorRole, "sensor"},
        {ValueRole, "value"},
        {SeverityRole, "severity"},
        {MessageRole, "message"},
        {TimestampRole, "timestamp"},
        {AcknowledgedRole, "acknowledged"},
    };
}

void AlarmModel::add_alarm(qint64 id, const QString& device_id,
                           const QString& sensor, double value,
                           const QString& severity, const QString& message,
                           const QString& timestamp) {
    beginInsertRows(QModelIndex(), alarms_.size(), alarms_.size());
    alarms_.append({id, device_id, sensor, value, severity, message, timestamp, false});
    endInsertRows();
    Q_EMIT alarmAdded();
}

void AlarmModel::acknowledge(qint64 alarm_id) {
    int row = find_alarm(alarm_id);
    if (row < 0) return;
    alarms_[row].acknowledged = true;
    auto idx = index(row);
    Q_EMIT dataChanged(idx, idx, {AcknowledgedRole});
    Q_EMIT alarmAcknowledged(alarm_id);
}

void AlarmModel::clear() {
    beginResetModel();
    alarms_.clear();
    endResetModel();
}

int AlarmModel::unacknowledged_count() const {
    int count = 0;
    for (const auto& a : alarms_) {
        if (!a.acknowledged) ++count;
    }
    return count;
}

int AlarmModel::find_alarm(qint64 id) const {
    for (int i = 0; i < alarms_.size(); ++i) {
        if (alarms_[i].id == id) return i;
    }
    return -1;
}

}  // namespace ui
