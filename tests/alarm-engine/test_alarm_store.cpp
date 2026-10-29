#include <gtest/gtest.h>

#include <libpq-fe.h>

#include <algorithm>

#include "alarm-engine/alarm_store.h"
#include "alarm-engine/types.h"

using namespace alarm_engine;

namespace {
std::string get_test_db() {
    const char* env = std::getenv("FORGESIGHT_TEST_DB");
    if (env)
        return env;
    return "dbname=forgesight_test";
}

bool db_available() {
    AlarmStoreConfig cfg{get_test_db()};
    AlarmStore store(cfg);
    return store.is_connected();
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

TEST_F(AlarmStoreTest, TryLoadRulesSeesThresholdUpdate) {
    AlarmStoreConfig cfg{conn_str};
    AlarmStore store(cfg);
    ASSERT_TRUE(store.is_connected());

    Rule r = make_rule();
    r.device_id = "reload-test";
    r.sensor = "vibration";
    r.threshold = 80.0;
    ASSERT_TRUE(store.add_rule(r));

    auto first = store.try_load_rules();
    ASSERT_TRUE(first.has_value());
    const Rule* newest = nullptr;
    for (const auto& rule : *first) {
        if (rule.device_id == "reload-test" && rule.sensor == "vibration" &&
            (newest == nullptr || rule.id > newest->id)) {
            newest = &rule;
        }
    }
    ASSERT_NE(newest, nullptr);
    EXPECT_DOUBLE_EQ(newest->threshold, 80.0);

    Rule edited = *newest;
    edited.threshold = 42.5;
    ASSERT_TRUE(store.update_rule(edited));

    auto second = store.try_load_rules();
    ASSERT_TRUE(second.has_value());
    auto updated = std::find_if(second->begin(), second->end(),
                                [&](const Rule& rule) { return rule.id == edited.id; });
    ASSERT_NE(updated, second->end());
    EXPECT_DOUBLE_EQ(updated->threshold, 42.5);
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

TEST_F(AlarmStoreTest, DeleteRuleWithFiredAlarmKeepsHistory) {
    // A rule that has already produced an alarm cannot be removed by a plain
    // DELETE: alarms.rule_id REFERENCES alarm_rules. The Rules tab hits that
    // path. History must stay readable, with a null rule id, after the delete.
    AlarmStoreConfig cfg{conn_str};
    AlarmStore store(cfg);
    ASSERT_TRUE(store.is_connected());

    Rule r = make_rule();
    r.device_id = "delete-fk-test";
    ASSERT_TRUE(store.add_rule(r));

    std::int64_t rule_id = -1;
    for (const auto& rule : store.load_rules()) {
        if (rule.device_id == "delete-fk-test")
            rule_id = rule.id;
    }
    ASSERT_GT(rule_id, 0);

    Alarm a = make_alarm();
    a.rule_id = rule_id;
    a.device_id = "delete-fk-test";
    ASSERT_TRUE(store.write_alarm(a));

    EXPECT_TRUE(store.delete_rule(rule_id));

    bool rule_gone = true;
    for (const auto& rule : store.load_rules()) {
        if (rule.id == rule_id)
            rule_gone = false;
    }
    EXPECT_TRUE(rule_gone);

    const auto alarms = store.load_alarms();
    const Alarm* kept = nullptr;
    for (const auto& alarm : alarms) {
        if (alarm.device_id == "delete-fk-test")
            kept = &alarm;
    }
    ASSERT_NE(kept, nullptr);
    EXPECT_EQ(kept->rule_id, 0);
    EXPECT_EQ(kept->message, a.message);

    auto* conn = PQconnectdb(conn_str.c_str());
    auto* res = PQexec(conn, "DELETE FROM alarms WHERE device_id = 'delete-fk-test'");
    PQclear(res);
    PQfinish(conn);
}

TEST_F(AlarmStoreTest, CreateTablesMigratesLegacyForeignKey) {
    // A database created before ON DELETE SET NULL keeps the default NO ACTION
    // constraint. create_tables must upgrade it, and a second run must be a no-op.
    auto* conn = PQconnectdb(conn_str.c_str());
    ASSERT_EQ(PQstatus(conn), CONNECTION_OK);
    auto exec = [&](const char* sql) {
        auto* res = PQexec(conn, sql);
        const bool ok = PQresultStatus(res) == PGRES_COMMAND_OK;
        PQclear(res);
        return ok;
    };
    ASSERT_TRUE(exec("ALTER TABLE alarms DROP CONSTRAINT IF EXISTS alarms_rule_id_fkey"));
    ASSERT_TRUE(exec("ALTER TABLE alarms ADD CONSTRAINT alarms_rule_id_fkey "
                     "FOREIGN KEY (rule_id) REFERENCES alarm_rules(id)"));

    auto delete_action = [&]() {
        auto* res = PQexec(conn, "SELECT confdeltype::text FROM pg_constraint "
                                 "WHERE conname = 'alarms_rule_id_fkey' "
                                 "AND conrelid = 'alarms'::regclass");
        std::string v;
        if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
            v = PQgetvalue(res, 0, 0);
        PQclear(res);
        return v;
    };
    ASSERT_EQ(delete_action(), "a"); // NO ACTION

    AlarmStoreConfig cfg{conn_str};
    AlarmStore store(cfg);
    ASSERT_TRUE(store.is_connected());
    EXPECT_TRUE(store.create_tables());
    EXPECT_EQ(delete_action(), "n"); // SET NULL
    EXPECT_TRUE(store.create_tables());
    EXPECT_EQ(delete_action(), "n");

    // A raw DELETE of a fired rule now succeeds and nulls the alarm's rule_id.
    Rule r = make_rule();
    r.device_id = "migrate-fk-test";
    ASSERT_TRUE(store.add_rule(r));
    std::int64_t rule_id = -1;
    for (const auto& rule : store.load_rules()) {
        if (rule.device_id == "migrate-fk-test")
            rule_id = rule.id;
    }
    ASSERT_GT(rule_id, 0);
    Alarm a = make_alarm();
    a.rule_id = rule_id;
    a.device_id = "migrate-fk-test";
    ASSERT_TRUE(store.write_alarm(a));

    const std::string del = "DELETE FROM alarm_rules WHERE id = " + std::to_string(rule_id);
    EXPECT_TRUE(exec(del.c_str()));
    EXPECT_TRUE(exec("DELETE FROM alarms WHERE device_id = 'migrate-fk-test'"));
    PQfinish(conn);
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

TEST_F(AlarmStoreTest, CreatesTimestampAndAcknowledgedIndexes) {
    AlarmStoreConfig cfg{conn_str};
    AlarmStore store(cfg);
    ASSERT_TRUE(store.is_connected());
    ASSERT_TRUE(store.create_tables());

    EXPECT_TRUE(index_exists(conn_str, "idx_alarms_timestamp"));
    EXPECT_TRUE(index_exists(conn_str, "idx_alarms_acknowledged"));
}

TEST_F(AlarmStoreTest, BadConnectionFailsGracefully) {
    AlarmStoreConfig cfg{"host=invalid-host dbname=nope"};
    AlarmStore store(cfg);
    EXPECT_FALSE(store.is_connected());
    EXPECT_FALSE(store.add_rule(make_rule()));
    EXPECT_TRUE(store.load_rules().empty());
}
