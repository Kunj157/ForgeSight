#include "api/device_service.h"

#include <libpq-fe.h>
#include <spdlog/spdlog.h>

namespace api {

DeviceService::DeviceService(void* conn) : conn_(conn) {
    ensure_metadata_table();
}

void DeviceService::ensure_metadata_table() const {
    if (!conn_)
        return;
    const char* ddl = "CREATE TABLE IF NOT EXISTS device_metadata ("
                      "device_id TEXT PRIMARY KEY, "
                      "plant TEXT NOT NULL DEFAULT 'Unassigned', "
                      "floor TEXT NOT NULL DEFAULT 'Unassigned')";
    auto* res = PQexec(static_cast<PGconn*>(conn_), ddl);
    PQclear(res);
}

bool DeviceService::set_device_location(const std::string& device_id, const std::string& plant,
                                        const std::string& floor) const {
    if (!conn_)
        return false;

    const char* params[3] = {device_id.c_str(), plant.c_str(), floor.c_str()};
    int lengths[3] = {static_cast<int>(device_id.size()), static_cast<int>(plant.size()),
                      static_cast<int>(floor.size())};
    int formats[3] = {0, 0, 0};

    auto* res = PQexecParams(static_cast<PGconn*>(conn_),
                             "INSERT INTO device_metadata (device_id, plant, floor) "
                             "VALUES ($1, $2, $3) "
                             "ON CONFLICT (device_id) DO UPDATE SET plant = EXCLUDED.plant, "
                             "floor = EXCLUDED.floor",
                             3, nullptr, params, lengths, formats, 0);

    bool ok = PQresultStatus(res) == PGRES_COMMAND_OK;
    PQclear(res);
    return ok;
}

bool DeviceService::seed_default_location(const std::string& device_id, const std::string& plant,
                                          const std::string& floor) const {
    if (!conn_)
        return false;

    const char* params[3] = {device_id.c_str(), plant.c_str(), floor.c_str()};
    int lengths[3] = {static_cast<int>(device_id.size()), static_cast<int>(plant.size()),
                      static_cast<int>(floor.size())};
    int formats[3] = {0, 0, 0};

    auto* res = PQexecParams(static_cast<PGconn*>(conn_),
                             "INSERT INTO device_metadata (device_id, plant, floor) "
                             "VALUES ($1, $2, $3) "
                             "ON CONFLICT (device_id) DO NOTHING",
                             3, nullptr, params, lengths, formats, 0);

    bool ok = PQresultStatus(res) == PGRES_COMMAND_OK;
    PQclear(res);
    return ok;
}

std::vector<DeviceInfo> DeviceService::list_devices() const {
    std::vector<DeviceInfo> devices;
    if (!conn_)
        return devices;

    // Latest reading per device+sensor so the dashboard can seed tiles;
    // left-joined against device_metadata so devices without an assigned
    // plant/floor still show up (grouped under "Unassigned").
    auto* res = PQexec(static_cast<PGconn*>(conn_),
                       "SELECT DISTINCT ON (r.device_id, r.sensor) "
                       "r.device_id, r.sensor, r.value, r.unit, r.timestamp::text, r.anomaly, "
                       "COALESCE(m.plant, 'Unassigned'), COALESCE(m.floor, 'Unassigned') "
                       "FROM readings r "
                       "LEFT JOIN device_metadata m ON m.device_id = r.device_id "
                       "ORDER BY r.device_id, r.sensor, r.timestamp DESC");

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return devices;
    }

    int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i) {
        DeviceInfo d;
        d.id = PQgetvalue(res, i, 0);
        d.name = d.id;
        d.sensor = PQgetvalue(res, i, 1);
        d.last_value = std::stod(PQgetvalue(res, i, 2));
        d.last_unit = PQgetvalue(res, i, 3);
        d.last_reading_time = PQgetvalue(res, i, 4);
        d.anomaly = (PQgetvalue(res, i, 5)[0] == 't');
        d.plant = PQgetvalue(res, i, 6);
        d.floor = PQgetvalue(res, i, 7);
        devices.push_back(std::move(d));
    }
    PQclear(res);
    return devices;
}

std::vector<ingestion::Reading> DeviceService::list_latest_readings() const {
    std::vector<ingestion::Reading> readings;
    auto devices = list_devices();
    readings.reserve(devices.size());
    for (const auto& d : devices) {
        ingestion::Reading r;
        r.device_id = d.id;
        r.sensor = d.sensor;
        r.value = d.last_value;
        r.unit = d.last_unit;
        r.timestamp = d.last_reading_time;
        r.anomaly = d.anomaly;
        readings.push_back(std::move(r));
    }
    return readings;
}

std::vector<ingestion::Reading> DeviceService::get_history(const std::string& device_id,
                                                           const std::string& sensor,
                                                           const std::string& since) const {

    std::vector<ingestion::Reading> readings;
    if (!conn_)
        return readings;

    const char* params[3] = {device_id.c_str(), sensor.c_str(), since.c_str()};
    int lengths[3] = {static_cast<int>(device_id.size()), static_cast<int>(sensor.size()),
                      static_cast<int>(since.size())};
    int formats[3] = {0, 0, 0};

    auto* res = PQexecParams(static_cast<PGconn*>(conn_),
                             "SELECT device_id, sensor, value, unit, timestamp::text, anomaly "
                             "FROM readings "
                             "WHERE device_id = $1 AND sensor = $2 AND timestamp >= $3 "
                             "ORDER BY timestamp",
                             3, nullptr, params, lengths, formats, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return readings;
    }

    int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i) {
        ingestion::Reading r;
        r.device_id = PQgetvalue(res, i, 0);
        r.sensor = PQgetvalue(res, i, 1);
        r.value = std::stod(PQgetvalue(res, i, 2));
        r.unit = PQgetvalue(res, i, 3);
        r.timestamp = PQgetvalue(res, i, 4);
        r.anomaly = (PQgetvalue(res, i, 5)[0] == 't');
        readings.push_back(std::move(r));
    }

    PQclear(res);
    return readings;
}

std::vector<ingestion::Reading> DeviceService::get_readings_since(const std::string& since) const {

    std::vector<ingestion::Reading> readings;
    if (!conn_)
        return readings;

    const char* params[1] = {since.c_str()};
    int lengths[1] = {static_cast<int>(since.size())};
    int formats[1] = {0};

    auto* res = PQexecParams(static_cast<PGconn*>(conn_),
                             "SELECT device_id, sensor, value, unit, timestamp::text, anomaly "
                             "FROM readings "
                             "WHERE timestamp >= $1 "
                             "ORDER BY timestamp",
                             1, nullptr, params, lengths, formats, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return readings;
    }

    int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i) {
        ingestion::Reading r;
        r.device_id = PQgetvalue(res, i, 0);
        r.sensor = PQgetvalue(res, i, 1);
        r.value = std::stod(PQgetvalue(res, i, 2));
        r.unit = PQgetvalue(res, i, 3);
        r.timestamp = PQgetvalue(res, i, 4);
        r.anomaly = (PQgetvalue(res, i, 5)[0] == 't');
        readings.push_back(std::move(r));
    }

    PQclear(res);
    return readings;
}

} // namespace api
