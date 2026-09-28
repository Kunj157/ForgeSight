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
    //
    // The DISTINCT ON runs on `readings` alone (the join is applied afterwards
    // to the handful of resulting rows). Kept this way — rather than joining
    // before the distinct — so the planner can satisfy
    // `ORDER BY device_id, sensor, timestamp DESC` with an index scan on
    // idx_readings_latest instead of a full-table Seq Scan + on-disk Sort.
    // The `::text` cast stays in the projection only; casting inside ORDER BY
    // would sort by text and defeat the index.
    auto* res = PQexec(static_cast<PGconn*>(conn_),
                       "SELECT l.device_id, l.sensor, l.value, l.unit, l.timestamp::text, "
                       "l.anomaly, "
                       "COALESCE(m.plant, 'Unassigned'), COALESCE(m.floor, 'Unassigned') "
                       "FROM ("
                       "  SELECT DISTINCT ON (device_id, sensor) "
                       "         device_id, sensor, value, unit, timestamp, anomaly "
                       "  FROM readings "
                       "  ORDER BY device_id, sensor, timestamp DESC"
                       ") l "
                       "LEFT JOIN device_metadata m ON m.device_id = l.device_id");

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
                                                           const std::string& since,
                                                           int max_points) const {

    std::vector<ingestion::Reading> readings;
    if (!conn_)
        return readings;

    const std::string max_points_str = std::to_string(max_points);
    const char* params[4] = {device_id.c_str(), sensor.c_str(), since.c_str(),
                             max_points_str.c_str()};
    int lengths[4] = {static_cast<int>(device_id.size()), static_cast<int>(sensor.size()),
                      static_cast<int>(since.size()), static_cast<int>(max_points_str.size())};
    int formats[4] = {0, 0, 0, 0};

    // Downsample to ~max_points evenly-spaced rows once the window exceeds the
    // cap, so a wide range doesn't ship (and the chart doesn't append) tens of
    // thousands of points. `count(*) OVER ()` gives the total so the stride is
    // ceil(total / max_points); we keep every stride-th row plus all anomalies
    // and the final row. max_points <= 0 skips this and returns everything.
    const char* sql_all = "SELECT device_id, sensor, value, unit, timestamp::text, anomaly "
                          "FROM readings "
                          "WHERE device_id = $1 AND sensor = $2 AND timestamp >= $3 "
                          "ORDER BY timestamp";
    const char* sql_downsampled =
        "SELECT device_id, sensor, value, unit, timestamp::text, anomaly FROM ("
        "  SELECT device_id, sensor, value, unit, timestamp, anomaly, "
        "         row_number() OVER (ORDER BY timestamp) AS rn, "
        "         count(*) OVER () AS total "
        "  FROM readings "
        "  WHERE device_id = $1 AND sensor = $2 AND timestamp >= $3"
        ") s "
        "WHERE total <= $4::int "
        "   OR ((rn - 1) % GREATEST(1, (total + $4::int - 1) / $4::int)) = 0 "
        "   OR anomaly "
        "   OR rn = total "
        "ORDER BY timestamp";

    auto* res = (max_points > 0) ? PQexecParams(static_cast<PGconn*>(conn_), sql_downsampled, 4,
                                                nullptr, params, lengths, formats, 0)
                                 : PQexecParams(static_cast<PGconn*>(conn_), sql_all, 3, nullptr,
                                                params, lengths, formats, 0);

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

std::optional<std::int64_t>
DeviceService::reading_poll_cursor(const std::string& catchup_since) const {
    if (!conn_)
        return std::nullopt;

    const char* params[1] = {catchup_since.c_str()};
    int lengths[1] = {static_cast<int>(catchup_since.size())};
    int formats[1] = {0};

    // Id just before the catch-up window. When the window is empty, the
    // second arm is the current max id, so the next poll does not replay
    // history. A failed lookup returns nullopt: id 0 would broadcast every
    // stored row once the table is reachable again.
    auto* res =
        PQexecParams(static_cast<PGconn*>(conn_),
                     "SELECT COALESCE("
                     "  (SELECT MIN(id) - 1 FROM readings WHERE timestamp >= $1::timestamptz),"
                     "  (SELECT COALESCE(MAX(id), 0) FROM readings)"
                     ")",
                     1, nullptr, params, lengths, formats, 0);

    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0 && !PQgetisnull(res, 0, 0)) {
        const auto cursor = std::stoll(PQgetvalue(res, 0, 0));
        PQclear(res);
        return cursor;
    }
    PQclear(res);

    auto* max_res =
        PQexec(static_cast<PGconn*>(conn_), "SELECT COALESCE(MAX(id), 0) FROM readings");
    std::optional<std::int64_t> cursor;
    if (PQresultStatus(max_res) == PGRES_TUPLES_OK && PQntuples(max_res) > 0 &&
        !PQgetisnull(max_res, 0, 0)) {
        cursor = std::stoll(PQgetvalue(max_res, 0, 0));
    }
    PQclear(max_res);
    return cursor;
}

std::vector<PolledReading> DeviceService::readings_after(std::int64_t after_id) const {
    std::vector<PolledReading> readings;
    if (!conn_)
        return readings;

    const std::string id_str = std::to_string(after_id);
    const char* params[1] = {id_str.c_str()};
    int lengths[1] = {static_cast<int>(id_str.size())};
    int formats[1] = {0};

    // Primary key order is the commit order of the single ingestion writer.
    // `id > cursor` includes a later row that shares the previous row's
    // timestamp, and excludes rows already broadcast.
    auto* res = PQexecParams(static_cast<PGconn*>(conn_),
                             "SELECT id, device_id, sensor, value, unit, timestamp::text, anomaly "
                             "FROM readings "
                             "WHERE id > $1::bigint "
                             "ORDER BY id",
                             1, nullptr, params, lengths, formats, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return readings;
    }

    int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i) {
        PolledReading row;
        row.id = std::stoll(PQgetvalue(res, i, 0));
        row.reading.device_id = PQgetvalue(res, i, 1);
        row.reading.sensor = PQgetvalue(res, i, 2);
        row.reading.value = std::stod(PQgetvalue(res, i, 3));
        row.reading.unit = PQgetvalue(res, i, 4);
        row.reading.timestamp = PQgetvalue(res, i, 5);
        row.reading.anomaly = (PQgetvalue(res, i, 6)[0] == 't');
        readings.push_back(std::move(row));
    }

    PQclear(res);
    return readings;
}

} // namespace api
