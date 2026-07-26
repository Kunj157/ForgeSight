#include <gtest/gtest.h>

#include "alarm_store.h"
#include "types.h"

using namespace alarm_engine;

namespace {
std::string get_test_db() {
    const char* env = std::getenv("FORGESIGHT_TEST_DB");
    if (env) return env;
    return "dbname=forgesight_test";
}

bool db_available() {
    AlarmStoreConfig cfg{get_test_db()};
    AlarmStore store(cfg);
    return store.is_connected();
}
}  // namespace

class AlarmStoreTest : public ::testing::Test {
protected:
    std::string conn_str = get_test_db();

    void SetUp() override {
        if (!db_available()) {
            GTEST_SKIP() << "Test DB not available";
        }
        AlarmStoreConfig cfg{conn_str};
        AlarmStore store(cfg);
        if (store.is_connected()) {
            store.create_tables();
        }
    }

    Rule make_rule() {
        Rule r;
        r.device_id = "pump-001";
        r.sensor = "temperature";
        r.condition = Condition::GreaterThan;
        r.threshold = 80.0;
        r.severity = Severity::Warning;
        r.message_template = "{device} {sensor} value {value} exceeded {threshold}";
        return r;
    }

    Alarm make_alarm() {
        Alarm a;
        a.rule_id = 1;
        a.device_id = "pump-001";
        a.sensor = "temperature";
        a.value = 95.0;
        a.severity = Severity::Warning;
        a.message = "pump-001 temperature value 95 exceeded 80";
        a.timestamp = "2026-07-26T10:00:00Z";
        a.acknowledged = false;
        return a;
    }
};

TEST_F(AlarmStoreTest, ConnectsToDb) {
    AlarmStoreConfig cfg{conn_str};
    AlarmStore store(cfg);
    EXPECT_TRUE(store.is_connected());
}

TEST_F(AlarmStoreTest, AddAndLoadRule) {
    AlarmStoreConfig cfg{conn_str};
    AlarmStore store(cfg);
    ASSERT_TRUE(store.is_connected());

    Rule r = make_rule();
    EXPECT_TRUE(store.add_rule(r));

    auto rules = store.load_rules();
    EXPECT_GE(rules.size(), 1u);
    EXPECT_EQ(rules.back().device_id, "pump-001");
    EXPECT_EQ(rules.back().sensor, "temperature");
}

TEST_F(AlarmStoreTest, DeleteRule) {
    AlarmStoreConfig cfg{conn_str};
    AlarmStore store(cfg);
    ASSERT_TRUE(store.is_connected());

    Rule r = make_rule();
    store.add_rule(r);
    auto rules = store.load_rules();
    ASSERT_FALSE(rules.empty());

    EXPECT_TRUE(store.delete_rule(rules.back().id));
    auto after = store.load_rules();
    EXPECT_LT(after.size(), rules.size());
}

TEST_F(AlarmStoreTest, WriteAndLoadAlarm) {
    AlarmStoreConfig cfg{conn_str};
    AlarmStore store(cfg);
    ASSERT_TRUE(store.is_connected());

    Alarm a = make_alarm();
    EXPECT_TRUE(store.write_alarm(a));

    auto alarms = store.load_alarms();
    EXPECT_GE(alarms.size(), 1u);
    EXPECT_EQ(alarms.back().device_id, "pump-001");
}

TEST_F(AlarmStoreTest, AcknowledgeAlarm) {
    AlarmStoreConfig cfg{conn_str};
    AlarmStore store(cfg);
    ASSERT_TRUE(store.is_connected());

    Alarm a = make_alarm();
    store.write_alarm(a);
    auto alarms = store.load_alarms();
    ASSERT_FALSE(alarms.empty());

    auto id = alarms.back().id;
    EXPECT_TRUE(store.acknowledge_alarm(id));

    auto unacked = store.load_alarms(true);
    for (const auto& al : unacked) {
        EXPECT_NE(al.id, id);
    }
}

TEST_F(AlarmStoreTest, BadConnectionFailsGracefully) {
    AlarmStoreConfig cfg{"host=invalid-host dbname=nope"};
    AlarmStore store(cfg);
    EXPECT_FALSE(store.is_connected());
    EXPECT_FALSE(store.add_rule(make_rule()));
    EXPECT_TRUE(store.load_rules().empty());
}
