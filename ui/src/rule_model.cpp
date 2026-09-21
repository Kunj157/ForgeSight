#include "ui/rule_model.h"

namespace ui {

RuleModel::RuleModel(QObject* parent) : QAbstractListModel(parent) {}

int RuleModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid())
        return 0;
    return static_cast<int>(rules_.size());
}

QVariant RuleModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= rules_.size())
        return {};

    const RuleEntry& r = rules_.at(index.row());
    switch (role) {
    case IdRole:
        return QVariant::fromValue(r.id);
    case DeviceIdRole:
        return r.device_id;
    case SensorRole:
        return r.sensor;
    case ConditionRole:
        return r.condition;
    case ThresholdRole:
        return r.threshold;
    case SeverityRole:
        return r.severity;
    default:
        return {};
    }
}

QHash<int, QByteArray> RuleModel::roleNames() const {
    return {
        {IdRole, "ruleId"},           {DeviceIdRole, "deviceId"},   {SensorRole, "sensor"},
        {ConditionRole, "condition"}, {ThresholdRole, "threshold"}, {SeverityRole, "severity"},
    };
}

void RuleModel::add_rule(qint64 id, const QString& device_id, const QString& sensor,
                         const QString& condition, double threshold, const QString& severity) {
    beginInsertRows(QModelIndex(), rules_.size(), rules_.size());
    rules_.push_back(RuleEntry{id, device_id, sensor, condition, threshold, severity});
    endInsertRows();
    Q_EMIT countChanged();
}

void RuleModel::clear() {
    if (rules_.isEmpty())
        return;
    beginResetModel();
    rules_.clear();
    endResetModel();
    Q_EMIT countChanged();
}

QVariantMap RuleModel::get(int row) const {
    QVariantMap map;
    if (row < 0 || row >= rules_.size())
        return map;

    const RuleEntry& r = rules_.at(row);
    map.insert("ruleId", QVariant::fromValue(r.id));
    map.insert("deviceId", r.device_id);
    map.insert("sensor", r.sensor);
    map.insert("condition", r.condition);
    map.insert("threshold", r.threshold);
    map.insert("severity", r.severity);
    return map;
}

} // namespace ui
