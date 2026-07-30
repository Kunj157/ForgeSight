#include "ingestion/db_writer.h"

#include <libpq-fe.h>
#include <spdlog/spdlog.h>

#include <stdexcept>

namespace ingestion {

DbWriter::DbWriter(const DbConfig& config) : config_(config) {
    conn_ = PQconnectdb(config.connection_string.c_str());
    if (PQstatus(static_cast<PGconn*>(conn_)) != CONNECTION_OK) {
        spdlog::warn("DB connection failed: {}", PQerrorMessage(static_cast<PGconn*>(conn_)));
        PQfinish(static_cast<PGconn*>(conn_));
        conn_ = nullptr;
        return;
    }
    ensure_table();
}

DbWriter::~DbWriter() {
    if (conn_) {
        flush();
        PQfinish(static_cast<PGconn*>(conn_));
    }
}

bool DbWriter::ensure_table() {
    const char* ddl = "CREATE TABLE IF NOT EXISTS readings ("
                      "  id BIGSERIAL PRIMARY KEY,"
                      "  device_id TEXT NOT NULL,"
                      "  sensor TEXT NOT NULL,"
                      "  value DOUBLE PRECISION NOT NULL,"
                      "  unit TEXT NOT NULL,"
                      "  timestamp TIMESTAMPTZ NOT NULL,"
                      "  anomaly BOOLEAN NOT NULL DEFAULT FALSE,"
                      "  created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()"
                      ")";

    auto* res = PQexec(static_cast<PGconn*>(conn_), ddl);
    bool ok = PQresultStatus(res) == PGRES_COMMAND_OK;
    PQclear(res);
    if (!ok) {
        spdlog::error("Failed to ensure readings table: {}",
                      PQerrorMessage(static_cast<PGconn*>(conn_)));
    }

    const char* index_ddl = "CREATE INDEX IF NOT EXISTS idx_readings_device_sensor_timestamp "
                            "ON readings (device_id, sensor, timestamp)";
    auto* idx_res = PQexec(static_cast<PGconn*>(conn_), index_ddl);
    bool idx_ok = PQresultStatus(idx_res) == PGRES_COMMAND_OK;
    PQclear(idx_res);
    if (!idx_ok) {
        spdlog::error("Failed to create readings index: {}",
                      PQerrorMessage(static_cast<PGconn*>(conn_)));
    }

    return ok && idx_ok;
}

bool DbWriter::is_connected() const {
    return conn_ != nullptr;
}

bool DbWriter::write(const Reading& reading) {
    if (!conn_)
        return false;
    std::lock_guard<std::mutex> lock(mutex_);
    buffer_.push_back(reading);
    if (buffer_.size() >= config_.batch_size) {
        return insert_batch(buffer_);
    }
    return true;
}

std::size_t DbWriter::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (buffer_.empty())
        return 0;
    std::size_t count = buffer_.size();
    insert_batch(buffer_);
    return count;
}

bool DbWriter::insert_batch(std::vector<Reading>& batch) {
    if (batch.empty() || !conn_)
        return true;

    std::string values;
    for (std::size_t i = 0; i < batch.size(); ++i) {
        const auto& r = batch[i];
        if (i > 0)
            values += ",";
        std::size_t off = i * 6;
        values += "($" + std::to_string(off + 1) +
                  ","
                  "$" +
                  std::to_string(off + 2) +
                  ","
                  "$" +
                  std::to_string(off + 3) +
                  ","
                  "$" +
                  std::to_string(off + 4) +
                  ","
                  "$" +
                  std::to_string(off + 5) +
                  ","
                  "$" +
                  std::to_string(off + 6) + ")";
    }

    std::string sql = "INSERT INTO readings (device_id, sensor, value, unit, timestamp, anomaly) "
                      "VALUES " +
                      values;

    std::vector<const char*> param_values;
    std::vector<int> param_lengths;
    std::vector<int> param_formats;
    std::vector<std::string> val_strs(batch.size());
    int idx = 0;

    for (const auto& r : batch) {
        val_strs[idx] = std::to_string(r.value);

        param_values.push_back(r.device_id.c_str());
        param_values.push_back(r.sensor.c_str());
        param_values.push_back(val_strs[idx].c_str());
        param_values.push_back(r.unit.c_str());
        param_values.push_back(r.timestamp.c_str());
        param_values.push_back(r.anomaly ? "t" : "f");

        param_lengths.push_back(static_cast<int>(r.device_id.size()));
        param_lengths.push_back(static_cast<int>(r.sensor.size()));
        param_lengths.push_back(static_cast<int>(val_strs[idx].size()));
        param_lengths.push_back(static_cast<int>(r.unit.size()));
        param_lengths.push_back(static_cast<int>(r.timestamp.size()));
        param_lengths.push_back(1);

        param_formats.push_back(0);
        param_formats.push_back(0);
        param_formats.push_back(0);
        param_formats.push_back(0);
        param_formats.push_back(0);
        param_formats.push_back(0);

        ++idx;
    }

    auto* res = PQexecParams(static_cast<PGconn*>(conn_), sql.c_str(),
                             static_cast<int>(param_values.size()), nullptr, param_values.data(),
                             param_lengths.data(), param_formats.data(), 0);

    bool ok = PQresultStatus(res) == PGRES_COMMAND_OK;
    if (!ok) {
        spdlog::error("Batch insert failed: {}", PQerrorMessage(static_cast<PGconn*>(conn_)));
    }
    PQclear(res);
    batch.clear();
    return ok;
}

} // namespace ingestion
