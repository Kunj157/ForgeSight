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
