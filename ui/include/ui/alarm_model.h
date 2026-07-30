#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QVector>

namespace ui {

struct AlarmEntry {
    qint64 id = 0;
    QString device_id;
    QString sensor;
    double value = 0.0;
    QString severity;
    QString message;
    QString timestamp;
    bool acknowledged = false;
};

class AlarmModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int unacknowledgedCount READ unacknowledged_count NOTIFY unacknowledgedCountChanged)

  public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        DeviceIdRole,
        SensorRole,
        ValueRole,
        SeverityRole,
        MessageRole,
        TimestampRole,
        AcknowledgedRole,
    };

    explicit AlarmModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return alarms_.size(); }

    Q_INVOKABLE void add_alarm(qint64 id, const QString& device_id, const QString& sensor,
                               double value, const QString& severity, const QString& message,
                               const QString& timestamp, bool acknowledged = false);
    Q_INVOKABLE void acknowledge(qint64 alarm_id);
    Q_INVOKABLE void clear();
    int unacknowledged_count() const;

  Q_SIGNALS:
    void alarmAdded();
    void alarmAcknowledged(qint64 alarm_id);
    void countChanged();
    void unacknowledgedCountChanged();

  private:
    int find_alarm(qint64 id) const;

    QVector<AlarmEntry> alarms_;
};

} // namespace ui
