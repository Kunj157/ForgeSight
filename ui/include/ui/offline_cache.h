#pragma once

#include <QList>
#include <QObject>
#include <QSqlDatabase>
#include <QString>
#include <QVector>

namespace ui {

struct CachedDeviceState {
    QString device_id;
    QString sensor;
    double value = 0.0;
    QString unit;
    QString timestamp;
    bool anomaly = false;
    QString plant;
    QString floor;
};

/// SQLite-backed client cache used for offline mode (Phase 7):
///  - last-known device readings, so the dashboard isn't empty on a cold
///    start with the backend unreachable.
///  - a queue of alarm acknowledgements made while offline, flushed once
///    the API is reachable again.
///
/// Every instance gets its own QSqlDatabase connection (a unique, generated
/// connection name) so multiple caches — e.g. across tests — can be open in
/// the same process without colliding on Qt's global connection registry.
class OfflineCache : public QObject {
    Q_OBJECT
    Q_PROPERTY(int pendingAckCount READ pendingAckCount NOTIFY pendingAckCountChanged)

  public:
    explicit OfflineCache(QObject* parent = nullptr);
    ~OfflineCache() override;

    /// Opens (creating if needed) the SQLite file at path. Safe to call
    /// again with a different path to switch files. Returns false if the
    /// database could not be opened; all other methods then become no-ops.
    Q_INVOKABLE bool open(const QString& path);

    /// Writable default location for the cache file (AppData/ForgeSight),
    /// not the current working directory.
    Q_INVOKABLE static QString default_path();

    Q_INVOKABLE void saveDeviceState(const QString& deviceId, const QString& sensor, double value,
                                     const QString& unit, const QString& timestamp, bool anomaly,
                                     const QString& plant, const QString& floor);
    QVector<CachedDeviceState> loadDeviceStates() const;

    Q_INVOKABLE void queueAck(qint64 alarmId);
    Q_INVOKABLE void clearAck(qint64 alarmId);
    QList<qint64> pendingAcks() const;
    int pendingAckCount() const;

  Q_SIGNALS:
    void pendingAckCountChanged();

  private:
    void ensure_schema();

    QSqlDatabase db_;
    QString connection_name_;
    bool open_ = false;
};

} // namespace ui
