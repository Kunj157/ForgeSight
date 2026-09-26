#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QVariantMap>
#include <QVector>

namespace ui {

struct RuleEntry {
    qint64 id = 0;
    QString device_id;
    QString sensor;
    QString condition; // "gt" | "lt" | "ge" | "le" | "eq" (API wire form)
    double threshold = 0.0;
    QString severity; // "info" | "warning" | "critical"
};

/// Backing model for the Rules tab. Populated from ApiClient::ruleReceived
/// (REST `GET /api/rules`); mutations go back out through ApiClient and the
/// list is refetched, so this model is intentionally a read-through cache.
class RuleModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

  public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        DeviceIdRole,
        SensorRole,
        ConditionRole,
        ThresholdRole,
        SeverityRole,
    };

    explicit RuleModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return static_cast<int>(rules_.size()); }

    Q_INVOKABLE void add_rule(qint64 id, const QString& device_id, const QString& sensor,
                              const QString& condition, double threshold, const QString& severity);
    Q_INVOKABLE void clear();

    /// Row as a QVariantMap keyed by the QML role names (ruleId, deviceId,
    /// sensor, condition, threshold, severity). Lets QML read a whole rule for
    /// the edit form without touching the role enum (which isn't Q_ENUM'd, so
    /// `RuleModel.DeviceIdRole` would read back undefined in QML). Out-of-range
    /// rows return an empty map.
    Q_INVOKABLE QVariantMap get(int row) const;

  Q_SIGNALS:
    void countChanged();

  private:
    QVector<RuleEntry> rules_;
};

} // namespace ui
