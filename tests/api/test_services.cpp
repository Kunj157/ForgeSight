#include <gtest/gtest.h>

#include <libpq-fe.h>

#include "alarm-engine/types.h"
#include "api/device_service.h"
#include "api/rule_service.h"

namespace {

std::string get_test_db() {
    const char* env = std::getenv("FORGESIGHT_TEST_DB");
    if (env)
        return env;
    return "dbname=forgesight_test";
}

void* connect_db() {
    auto* conn = PQconnectdb(get_test_db().c_str());
    if (PQstatus(conn) != CONNECTION_OK) {
        PQfinish(conn);
        return nullptr;
    }
    return conn;
}

void seed_reading(void* conn, const std::string& device, const std::string& sensor, double value,
                  const std::string& ts) {
    std::string value_str = std::to_string(value);
    const char* params[4] = {device.c_str(), sensor.c_str(), value_str.c_str(), ts.c_str()};
    int lengths[4] = {static_cast<int>(device.size()), static_cast<int>(sensor.size()),
                      static_cast<int>(value_str.size()), static_cast<int>(ts.size())};
    int formats[4] = {0, 0, 0, 0};
    auto* res =
        PQexecParams(static_cast<PGconn*>(conn),
                     "INSERT INTO readings (device_id, sensor, value, unit, timestamp, anomaly) "
                     "VALUES ($1, $2, $3, '°C', $4, false)",
                     4, nullptr, params, lengths, formats, 0);
    PQclear(res);
}

void exec_and_clear(void* conn, const char* sql) {
    auto* res = PQexec(static_cast<PGconn*>(conn), sql);
    PQclear(res);
}

void seed_rule(void* conn, const std::string& device, const std::string& sensor) {
    const char* params[2] = {device.c_str(), sensor.c_str()};
    int lengths[2] = {static_cast<int>(device.size()), static_cast<int>(sensor.size())};
    int formats[2] = {0, 0};
    auto* res =
        PQexecParams(static_cast<PGconn*>(conn),
                     "INSERT INTO alarm_rules (device_id, sensor, condition, threshold, severity) "
                     "VALUES ($1, $2, 'gt', 80.0, 'warning')",
                     2, nullptr, params, lengths, formats, 0);
    PQclear(res);
}

} // namespace

class DeviceServiceTest : public ::testing::Test {
  protected:
    void* conn = nullptr;

    void SetUp() override {
        conn = connect_db();
        if (!conn)
            GTEST_SKIP() << "Test DB not available";
    }

    void TearDown() override {
        if (conn) {
            exec_and_clear(conn, "DELETE FROM readings WHERE device_id = 'svc-test'");
            exec_and_clear(conn, "DELETE FROM alarm_rules WHERE device_id = 'svc-test'");
            exec_and_clear(conn, "DELETE FROM device_metadata WHERE device_id = 'svc-test'");
            PQfinish(static_cast<PGconn*>(conn));
        }
    }
};

TEST_F(DeviceServiceTest, ListDevicesReturnsFromReadings) {
    seed_reading(conn, "svc-test", "temperature", 65.0, "2026-07-26T10:00:00Z");
    api::DeviceService svc(conn);
    auto devices = svc.list_devices();

    bool found = false;
    for (const auto& d : devices) {
        if (d.id == "svc-test") {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(DeviceServiceTest, ListLatestReadingsReturnsPerSensor) {
    seed_reading(conn, "svc-test", "temperature", 65.0, "2026-07-26T10:00:00Z");
    seed_reading(conn, "svc-test", "pressure", 2.5, "2026-07-26T10:00:01Z");
    seed_reading(conn, "svc-test", "temperature", 70.0, "2026-07-26T10:00:02Z");

    api::DeviceService svc(conn);
    auto latest = svc.list_latest_readings();

    int temp = 0, pressure = 0;
    for (const auto& r : latest) {
        if (r.device_id != "svc-test")
            continue;
        if (r.sensor == "temperature") {
            ++temp;
            EXPECT_DOUBLE_EQ(r.value, 70.0);
        }
        if (r.sensor == "pressure") {
            ++pressure;
            EXPECT_DOUBLE_EQ(r.value, 2.5);
        }
    }
    EXPECT_EQ(temp, 1);
    EXPECT_EQ(pressure, 1);
}

TEST_F(DeviceServiceTest, GetHistoryReturnsReadings) {
    seed_reading(conn, "svc-test", "temperature", 65.0, "2026-07-26T10:00:00Z");
    seed_reading(conn, "svc-test", "temperature", 70.0, "2026-07-26T10:01:00Z");

    api::DeviceService svc(conn);
    auto readings = svc.get_history("svc-test", "temperature", "2026-07-26T00:00:00Z");
    EXPECT_GE(readings.size(), 2u);
}

TEST_F(DeviceServiceTest, GetHistoryFiltersBySensor) {
    seed_reading(conn, "svc-test", "temperature", 65.0, "2026-07-26T10:00:00Z");
    seed_reading(conn, "svc-test", "pressure", 2.5, "2026-07-26T10:00:00Z");

    api::DeviceService svc(conn);
    auto readings = svc.get_history("svc-test", "temperature", "2026-07-26T00:00:00Z");
    for (const auto& r : readings) {
        EXPECT_EQ(r.sensor, "temperature");
    }
}

TEST_F(DeviceServiceTest, NullConnReturnsEmpty) {
    api::DeviceService svc(nullptr);
    EXPECT_TRUE(svc.list_devices().empty());
    EXPECT_TRUE(svc.get_history("a", "b", "c").empty());
}

// Regression guard for the dashboard-bootstrap perf bug fixed in v1.1.0 (#46).
// The "latest reading per device+sensor" query behind list_devices() must be
// servable by idx_readings_latest with NO Sort step — at volume the pre-fix
// query did a full-table Seq Scan + on-disk Sort (~5.3s), which froze the
// single-threaded API event loop and timed out concurrent bootstrap requests
// (e.g. /api/alarms).
//
// The whole point of idx_readings_latest being `(device_id, sensor, timestamp
// DESC)` is that it already provides the exact order the DISTINCT ON needs, so
// the plan is `Unique -> Index Scan` with no separate Sort. We assert that with
// enable_seqscan disabled: this is deterministic (unlike the raw cost-based
// choice on small tables, which flips between Seq Scan+Sort and Index Scan) and
// still catches the real regression — if the index is dropped or its column
// order / DESC is changed, the planner is forced back into a Sort and the test
// fails.
//
// Self-contained: creates the table + index idempotently rather than relying on
// another test binary having done so, since ctest runs every suite in one pass
// with no guaranteed ordering. Verified on PostgreSQL 16.15 (== CI's postgres:16).
TEST_F(DeviceServiceTest, LatestReadingQueryIsIndexBackedNoSort) {
    exec_and_clear(
        conn,
        "CREATE TABLE IF NOT EXISTS readings ("
        "  id BIGSERIAL PRIMARY KEY, device_id TEXT NOT NULL, sensor TEXT NOT NULL,"
        "  value DOUBLE PRECISION NOT NULL, unit TEXT NOT NULL, timestamp TIMESTAMPTZ NOT NULL,"
        "  anomaly BOOLEAN NOT NULL DEFAULT FALSE, created_at TIMESTAMPTZ NOT NULL DEFAULT NOW())");
    exec_and_clear(conn, "CREATE INDEX IF NOT EXISTS idx_readings_latest "
                         "ON readings (device_id, sensor, timestamp DESC)");

    // Enough volume for a meaningful latest-per-group check, in one server-side
    // statement: 2k timestamps x 3 sensors = 6k rows.
    exec_and_clear(conn,
                   "INSERT INTO readings (device_id, sensor, value, unit, timestamp, anomaly) "
                   "SELECT 'svc-test', s.sensor, random() * 100, 'u', "
                   "       TIMESTAMPTZ '2026-07-26 10:00:00Z' + (g || ' seconds')::interval, false "
                   "FROM generate_series(1, 2000) g "
                   "CROSS JOIN (VALUES ('temperature'),('pressure'),('vibration')) AS s(sensor)");

    // Deterministic latest marker per sensor (far-future timestamp, known value).
    seed_reading(conn, "svc-test", "temperature", 42.5, "2026-07-26T20:00:00Z");
    seed_reading(conn, "svc-test", "pressure", 7.25, "2026-07-26T20:00:00Z");
    seed_reading(conn, "svc-test", "vibration", 3.5, "2026-07-26T20:00:00Z");

    exec_and_clear(conn, "ANALYZE readings");

    // (1) Correctness at volume: latest-per-sensor returns exactly the marker rows.
    api::DeviceService svc(conn);
    auto latest = svc.list_latest_readings();
    int temp = 0, pressure = 0, vibration = 0;
    for (const auto& r : latest) {
        if (r.device_id != "svc-test")
            continue;
        if (r.sensor == "temperature") {
            ++temp;
            EXPECT_DOUBLE_EQ(r.value, 42.5);
        } else if (r.sensor == "pressure") {
            ++pressure;
            EXPECT_DOUBLE_EQ(r.value, 7.25);
        } else if (r.sensor == "vibration") {
            ++vibration;
            EXPECT_DOUBLE_EQ(r.value, 3.5);
        }
    }
    EXPECT_EQ(temp, 1);
    EXPECT_EQ(pressure, 1);
    EXPECT_EQ(vibration, 1);

    // (2) Perf shape: idx_readings_latest must be able to satisfy the DISTINCT ON
    // ordering without a Sort. With seqscan disabled the planner is forced onto
    // the index if (and only if) the index actually provides that order. This
    // mirrors the exact subquery in DeviceService::list_devices().
    exec_and_clear(conn, "SET enable_seqscan = off");
    auto* res = PQexec(static_cast<PGconn*>(conn),
                       "EXPLAIN SELECT DISTINCT ON (device_id, sensor) "
                       "  device_id, sensor, value, unit, timestamp, anomaly "
                       "FROM readings ORDER BY device_id, sensor, timestamp DESC");
    ASSERT_EQ(PQresultStatus(res), PGRES_TUPLES_OK);
    std::string plan;
    for (int i = 0; i < PQntuples(res); ++i) {
        plan += PQgetvalue(res, i, 0);
        plan += '\n';
    }
    PQclear(res);
    exec_and_clear(conn, "SET enable_seqscan = on");

    EXPECT_NE(plan.find("idx_readings_latest"), std::string::npos)
        << "latest-reading query should be served by idx_readings_latest. Plan:\n"
        << plan;
    EXPECT_EQ(plan.find("Sort"), std::string::npos)
        << "idx_readings_latest should satisfy the ordering with no Sort. Plan:\n"
        << plan;
}

TEST_F(DeviceServiceTest, DeviceWithoutLocationDefaultsToUnassigned) {
    seed_reading(conn, "svc-test", "temperature", 65.0, "2026-07-26T10:00:00Z");
    api::DeviceService svc(conn);
    auto devices = svc.list_devices();

    bool found = false;
    for (const auto& d : devices) {
        if (d.id != "svc-test")
            continue;
        found = true;
        EXPECT_EQ(d.plant, "Unassigned");
        EXPECT_EQ(d.floor, "Unassigned");
    }
    EXPECT_TRUE(found);
}

TEST_F(DeviceServiceTest, SetDeviceLocationThenListDevicesReflectsIt) {
    seed_reading(conn, "svc-test", "temperature", 65.0, "2026-07-26T10:00:00Z");
    api::DeviceService svc(conn);
    EXPECT_TRUE(svc.set_device_location("svc-test", "Plant A", "Floor 2"));

    auto devices = svc.list_devices();
    bool found = false;
    for (const auto& d : devices) {
        if (d.id != "svc-test")
            continue;
        found = true;
        EXPECT_EQ(d.plant, "Plant A");
        EXPECT_EQ(d.floor, "Floor 2");
    }
    EXPECT_TRUE(found);
}

TEST_F(DeviceServiceTest, SeedDefaultLocationDoesNotOverwriteExisting) {
    seed_reading(conn, "svc-test", "temperature", 65.0, "2026-07-26T10:00:00Z");
    api::DeviceService svc(conn);
    svc.set_device_location("svc-test", "Plant A", "Floor 1");
    svc.seed_default_location("svc-test", "Plant Z", "Floor Z");

    auto devices = svc.list_devices();
    for (const auto& d : devices) {
        if (d.id == "svc-test") {
            EXPECT_EQ(d.plant, "Plant A");
            EXPECT_EQ(d.floor, "Floor 1");
        }
    }
}

TEST_F(DeviceServiceTest, SeedDefaultLocationAssignsWhenUnset) {
    seed_reading(conn, "svc-test", "temperature", 65.0, "2026-07-26T10:00:00Z");
    api::DeviceService svc(conn);
    svc.seed_default_location("svc-test", "Plant A", "Floor 1");

    auto devices = svc.list_devices();
    for (const auto& d : devices) {
        if (d.id == "svc-test") {
            EXPECT_EQ(d.plant, "Plant A");
            EXPECT_EQ(d.floor, "Floor 1");
        }
    }
}

TEST_F(DeviceServiceTest, SetDeviceLocationUpsertsOnRepeatedCalls) {
    seed_reading(conn, "svc-test", "temperature", 65.0, "2026-07-26T10:00:00Z");
    api::DeviceService svc(conn);
    EXPECT_TRUE(svc.set_device_location("svc-test", "Plant A", "Floor 1"));
    EXPECT_TRUE(svc.set_device_location("svc-test", "Plant A", "Floor 3"));

    auto devices = svc.list_devices();
    for (const auto& d : devices) {
        if (d.id == "svc-test")
            EXPECT_EQ(d.floor, "Floor 3");
    }
}

class RuleServiceTest : public ::testing::Test {
  protected:
    void* conn = nullptr;

    void SetUp() override {
        conn = connect_db();
        if (!conn)
            GTEST_SKIP() << "Test DB not available";
    }

    void TearDown() override {
        if (conn) {
            exec_and_clear(conn, "DELETE FROM alarm_rules WHERE device_id = 'svc-test'");
            PQfinish(static_cast<PGconn*>(conn));
        }
    }
};

TEST_F(RuleServiceTest, CreateAndListRules) {
    api::RuleService svc(conn);
    alarm_engine::Rule r;
    r.device_id = "svc-test";
    r.sensor = "temperature";
    r.condition = alarm_engine::Condition::GreaterThan;
    r.threshold = 80.0;
    r.severity = alarm_engine::Severity::Warning;

    auto id = svc.create_rule(r);
    EXPECT_GT(id, 0);

    auto rules = svc.list_rules();
    bool found = false;
    for (const auto& rule : rules) {
        if (rule.id == id) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(RuleServiceTest, GetSingleRule) {
    api::RuleService svc(conn);
    alarm_engine::Rule r;
    r.device_id = "svc-test";
    r.sensor = "temperature";
    r.condition = alarm_engine::Condition::LessThan;
    r.threshold = 10.0;
    r.severity = alarm_engine::Severity::Critical;

    auto id = svc.create_rule(r);
    auto fetched = svc.get_rule(id);
    ASSERT_TRUE(fetched.has_value());
    EXPECT_EQ(fetched->device_id, "svc-test");
    EXPECT_DOUBLE_EQ(fetched->threshold, 10.0);
}

TEST_F(RuleServiceTest, DeleteRule) {
    api::RuleService svc(conn);
    alarm_engine::Rule r;
    r.device_id = "svc-test";
    r.sensor = "temperature";
    r.condition = alarm_engine::Condition::GreaterThan;
    r.threshold = 90.0;
    r.severity = alarm_engine::Severity::Warning;

    auto id = svc.create_rule(r);
    EXPECT_TRUE(svc.delete_rule(id));
    EXPECT_FALSE(svc.get_rule(id).has_value());
}

TEST_F(RuleServiceTest, NullConnReturnsEmpty) {
    api::RuleService svc(nullptr);
    EXPECT_TRUE(svc.list_rules().empty());
    EXPECT_FALSE(svc.get_rule(1).has_value());
    EXPECT_EQ(svc.create_rule({}), -1);
}
