#include "api/device_service.h"

#include <libpq-fe.h>
#include <spdlog/spdlog.h>

namespace api {

DeviceService::DeviceService(void* conn) : conn_(conn) {}

std::vector<DeviceInfo> DeviceService::list_devices() const {
    std::vector<DeviceInfo> devices;
    if (!conn_) return devices;

    auto* res = PQexec(static_cast<PGconn*>(conn_),
        "SELECT DISTINCT device_id FROM readings ORDER BY device_id");

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return devices;
    }

    int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i) {
        DeviceInfo d;
        d.id = PQgetvalue(res, i, 0);
        devices.push_back(std::move(d));
    }
    PQclear(res);
    return devices;
}

std::vector<ingestion::Reading> DeviceService::get_history(
    const std::string& device_id,
    const std::string& sensor,
    const std::string& since) const {

    std::vector<ingestion::Reading> readings;
    if (!conn_) return readings;

    const char* params[3] = {device_id.c_str(), sensor.c_str(), since.c_str()};
    int lengths[3] = {
        static_cast<int>(device_id.size()),
        static_cast<int>(sensor.size()),
        static_cast<int>(since.size())
    };
    int formats[3] = {0, 0, 0};

    auto* res = PQexecParams(
        static_cast<PGconn*>(conn_),
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

}  // namespace api
