#include "alarm-engine/reading_cursor.h"

#include <exception>
#include <string>

#include <libpq-fe.h>
#include <spdlog/spdlog.h>

namespace alarm_engine {

std::vector<PolledReading> fetch_readings_after(void* conn, std::int64_t after_id) {
    std::vector<PolledReading> readings;
    if (!conn)
        return readings;

    const auto id_str = std::to_string(after_id);
    const char* params[1] = {id_str.c_str()};
    int lens[1] = {static_cast<int>(id_str.size())};
    int fmts[1] = {0};

    auto* res = PQexecParams(static_cast<PGconn*>(conn),
                             "SELECT id, device_id, sensor, value, unit, timestamp::text, anomaly "
                             "FROM readings WHERE id > $1::bigint ORDER BY id",
                             1, nullptr, params, lens, fmts, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        spdlog::error("Failed to fetch readings after id {}: {}", after_id,
                      PQerrorMessage(static_cast<PGconn*>(conn)));
        PQclear(res);
        return readings;
    }

    int rows = PQntuples(res);
    readings.reserve(static_cast<std::size_t>(rows));
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

std::optional<std::int64_t> catch_up_reading_id(void* conn) {
    if (!conn)
        return std::nullopt;

    auto* res =
        PQexec(static_cast<PGconn*>(conn), "SELECT COALESCE(MAX(id), 0)::text FROM readings");
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        spdlog::error("Failed to read readings cursor: {}",
                      PQerrorMessage(static_cast<PGconn*>(conn)));
        PQclear(res);
        return std::nullopt;
    }

    std::int64_t id = 0;
    try {
        id = std::stoll(PQgetvalue(res, 0, 0));
    } catch (const std::exception& e) {
        spdlog::error("Invalid readings cursor: {}", e.what());
        PQclear(res);
        return std::nullopt;
    }
    PQclear(res);
    return id;
}

} // namespace alarm_engine
