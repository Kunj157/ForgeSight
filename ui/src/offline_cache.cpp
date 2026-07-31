#include "ui/offline_cache.h"

#include <QAtomicInt>
#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>

namespace ui {

namespace {
QAtomicInt g_connection_counter{0};
}

OfflineCache::OfflineCache(QObject* parent) : QObject(parent) {
    connection_name_ = QStringLiteral("forgesight_offline_cache_%1")
                           .arg(g_connection_counter.fetchAndAddOrdered(1));
}

OfflineCache::~OfflineCache() {
    if (db_.isValid()) {
        db_.close();
        // Drop our own handle to the connection before removeDatabase() —
        // otherwise Qt sees db_ itself as still referencing it (member
        // destruction happens after this body runs) and only warns instead
        // of actually freeing it.
        db_ = QSqlDatabase();
        QSqlDatabase::removeDatabase(connection_name_);
    }
}

bool OfflineCache::open(const QString& path) {
    if (db_.isValid()) {
        db_.close();
        db_ = QSqlDatabase();
        QSqlDatabase::removeDatabase(connection_name_);
    }

    db_ = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection_name_);
    db_.setDatabaseName(path);
    open_ = db_.open();
    if (open_) {
        ensure_schema();
    }
    return open_;
}

QString OfflineCache::default_path() {
    QString dirPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dirPath.isEmpty())
        dirPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) +
                  QStringLiteral("/.forgesight");
    QDir().mkpath(dirPath);
    return dirPath + QStringLiteral("/offline_cache.db");
}

void OfflineCache::ensure_schema() {
    QSqlQuery query(db_);
    query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS device_states ("
                              "device_id TEXT NOT NULL, "
                              "sensor TEXT NOT NULL, "
                              "value REAL NOT NULL, "
                              "unit TEXT, "
                              "timestamp TEXT, "
                              "anomaly INTEGER NOT NULL DEFAULT 0, "
                              "plant TEXT, "
                              "floor TEXT, "
                              "PRIMARY KEY (device_id, sensor))"));
    query.exec(
        QStringLiteral("CREATE TABLE IF NOT EXISTS pending_acks (alarm_id INTEGER PRIMARY KEY)"));
}

void OfflineCache::saveDeviceState(const QString& deviceId, const QString& sensor, double value,
                                   const QString& unit, const QString& timestamp, bool anomaly,
                                   const QString& plant, const QString& floor) {
    if (!open_)
        return;

    QSqlQuery query(db_);
    query.prepare(QStringLiteral(
        "INSERT INTO device_states (device_id, sensor, value, unit, timestamp, anomaly, plant, "
        "floor) VALUES (?, ?, ?, ?, ?, ?, ?, ?) "
        "ON CONFLICT (device_id, sensor) DO UPDATE SET "
        "value = excluded.value, unit = excluded.unit, timestamp = excluded.timestamp, "
        "anomaly = excluded.anomaly, plant = excluded.plant, floor = excluded.floor"));
    query.addBindValue(deviceId);
    query.addBindValue(sensor);
    query.addBindValue(value);
    query.addBindValue(unit);
    query.addBindValue(timestamp);
    query.addBindValue(anomaly ? 1 : 0);
    query.addBindValue(plant);
    query.addBindValue(floor);
    query.exec();
}

QVector<CachedDeviceState> OfflineCache::loadDeviceStates() const {
    QVector<CachedDeviceState> result;
    if (!open_)
        return result;

    QSqlQuery query(db_);
    query.exec(
        QStringLiteral("SELECT device_id, sensor, value, unit, timestamp, anomaly, plant, floor "
                       "FROM device_states"));
    while (query.next()) {
        CachedDeviceState s;
        s.device_id = query.value(0).toString();
        s.sensor = query.value(1).toString();
        s.value = query.value(2).toDouble();
        s.unit = query.value(3).toString();
        s.timestamp = query.value(4).toString();
        s.anomaly = query.value(5).toInt() != 0;
        s.plant = query.value(6).toString();
        s.floor = query.value(7).toString();
        result.push_back(std::move(s));
    }
    return result;
}

void OfflineCache::queueAck(qint64 alarmId) {
    if (!open_)
        return;

    QSqlQuery query(db_);
    query.prepare(QStringLiteral("INSERT OR IGNORE INTO pending_acks (alarm_id) VALUES (?)"));
    query.addBindValue(alarmId);
    query.exec();
    if (query.numRowsAffected() > 0) {
        Q_EMIT pendingAckCountChanged();
    }
}

void OfflineCache::clearAck(qint64 alarmId) {
    if (!open_)
        return;

    QSqlQuery query(db_);
    query.prepare(QStringLiteral("DELETE FROM pending_acks WHERE alarm_id = ?"));
    query.addBindValue(alarmId);
    query.exec();
    if (query.numRowsAffected() > 0) {
        Q_EMIT pendingAckCountChanged();
    }
}

QList<qint64> OfflineCache::pendingAcks() const {
    QList<qint64> result;
    if (!open_)
        return result;

    QSqlQuery query(db_);
    query.exec(QStringLiteral("SELECT alarm_id FROM pending_acks ORDER BY alarm_id"));
    while (query.next()) {
        result.append(query.value(0).toLongLong());
    }
    return result;
}

int OfflineCache::pendingAckCount() const {
    return pendingAcks().size();
}

} // namespace ui
