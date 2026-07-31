#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QPair>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

namespace ui {

struct DeviceState {
    QString device_id;
    QString sensor;
    double value = 0.0;
    QString unit;
    QString timestamp;
    bool anomaly = false;
    QString status; // "normal", "warning", "critical"
    QString plant = QStringLiteral("Unassigned");
    QString floor = QStringLiteral("Unassigned");
};

class DeviceModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

  public:
    enum Roles {
        DeviceIdRole = Qt::UserRole + 1,
        SensorRole,
        ValueRole,
        UnitRole,
        TimestampRole,
        AnomalyRole,
        StatusRole,
        PlantRole,
        FloorRole,
    };

    explicit DeviceModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return devices_.size(); }

    Q_INVOKABLE void updateDevice(const QString& device_id, const QString& sensor, double value,
                                  const QString& unit, const QString& timestamp, bool anomaly);
    void clear();

    Q_INVOKABLE QString device_status(const QString& device_id) const;

    /// Sets/updates the plant+floor location for a device. Applies immediately
    /// to any existing rows for that device (regardless of sensor) and is
    /// remembered so future updateDevice() calls for that device_id pick it up
    /// too — metadata (REST, per-device) and readings (WS, per-device+sensor)
    /// can arrive in either order.
    Q_INVOKABLE void updateDeviceMeta(const QString& device_id, const QString& plant,
                                      const QString& floor);

    Q_INVOKABLE QStringList plants() const;
    Q_INVOKABLE QStringList floors(const QString& plant) const;
    Q_INVOKABLE QVariantList devicesFor(const QString& plant, const QString& floor) const;

  Q_SIGNALS:
    void countChanged();

  private:
    int find_device(const QString& device_id, const QString& sensor) const;
    static QString compute_status(bool anomaly, double value);
    QVariantMap row_to_map(const DeviceState& d) const;

    QVector<DeviceState> devices_;
    QHash<QString, QPair<QString, QString>> meta_; // device_id -> (plant, floor)
};

} // namespace ui
