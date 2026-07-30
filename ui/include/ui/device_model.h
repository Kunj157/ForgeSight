#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QVector>

namespace ui {

struct DeviceState {
    QString device_id;
    QString sensor;
    double value = 0.0;
    QString unit;
    QString timestamp;
    bool anomaly = false;
    QString status;  // "normal", "warning", "critical"
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
    };

    explicit DeviceModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return devices_.size(); }

    Q_INVOKABLE void updateDevice(const QString& device_id, const QString& sensor,
                                   double value, const QString& unit,
                                   const QString& timestamp, bool anomaly);
    void clear();

    Q_INVOKABLE QString device_status(const QString& device_id) const;

Q_SIGNALS:
    void countChanged();

private:
    int find_device(const QString& device_id, const QString& sensor) const;
    static QString compute_status(bool anomaly, double value);

    QVector<DeviceState> devices_;
};

}  // namespace ui
