#include <gtest/gtest.h>

#include <libpq-fe.h>

#include <cstdlib>

#include "ingestion/db_writer.h"

using namespace ingestion;

namespace {

std::string get_test_conn_string() {
    const char* env = std::getenv("FORGESIGHT_TEST_DB");
    if (env)
        return env;
    return "dbname=forgesight_test";
}

bool index_exists(const std::string& conn_str, const std::string& index_name) {
    auto* conn = PQconnectdb(conn_str.c_str());
    if (PQstatus(conn) != CONNECTION_OK) {
        PQfinish(conn);
        return false;
    }

    const char* param_values[1] = {index_name.c_str()};
    auto* res = PQexecParams(conn, "SELECT 1 FROM pg_indexes WHERE indexname = $1", 1, nullptr,
                             param_values, nullptr, nullptr, 0);
    bool found = PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0;
    PQclear(res);
    PQfinish(conn);
    return found;
}

} // namespace

class DbWriterTest : public ::testing::Test {
  protected:
    std::string conn_str = get_test_conn_string();

    void SetUp() override {
        DbConfig cfg{conn_str};
        DbWriter w(cfg);
        if (!w.is_connected()) {
            GTEST_SKIP() << "Test DB not available at: " << conn_str;
        }
    }

    Reading make_reading(const std::string& device = "pump-001",
                         const std::string& sensor = "temperature", double value = 65.0) {
        Reading r;
        r.device_id = device;
        r.sensor = sensor;
        r.value = value;
        r.unit = "°C";
        r.timestamp = "2026-07-26T10:00:00Z";
        r.anomaly = false;
        return r;
    }
};

TEST_F(DbWriterTest, ConnectsToDb) {
    DbConfig cfg{conn_str};
    DbWriter w(cfg);
    EXPECT_TRUE(w.is_connected());
}

TEST_F(DbWriterTest, WriteSingleReading) {
    DbConfig cfg{conn_str};
    DbWriter w(cfg);
    ASSERT_TRUE(w.is_connected());

    EXPECT_TRUE(w.write(make_reading()));
    EXPECT_EQ(w.flush(), 1u);
}

TEST_F(DbWriterTest, FlushEmptyBuffer) {
    DbConfig cfg{conn_str};
    DbWriter w(cfg);
    EXPECT_TRUE(w.is_connected());
    EXPECT_EQ(w.flush(), 0u);
}

TEST_F(DbWriterTest, BatchInsert) {
    DbConfig cfg{conn_str};
    cfg.batch_size = 10;
    DbWriter w(cfg);
    ASSERT_TRUE(w.is_connected());

    for (int i = 0; i < 5; ++i) {
        w.write(make_reading("pump-001", "temperature", 60.0 + i));
    }
    EXPECT_EQ(w.flush(), 5u);
}

TEST_F(DbWriterTest, CreatesIndexOnDeviceSensorTimestamp) {
    DbConfig cfg{conn_str};
    DbWriter w(cfg);
    ASSERT_TRUE(w.is_connected());

    EXPECT_TRUE(index_exists(conn_str, "idx_readings_device_sensor_timestamp"));
}

TEST_F(DbWriterTest, BadConnectionFailsGracefully) {
    DbConfig cfg{"host=invalid-host-that-does-not-exist dbname=nope"};
    DbWriter w(cfg);
    EXPECT_FALSE(w.is_connected());
    EXPECT_FALSE(w.write(make_reading()));
}
