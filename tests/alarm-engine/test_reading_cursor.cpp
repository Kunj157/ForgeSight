#include <gtest/gtest.h>

#include <libpq-fe.h>

#include <string>

#include "alarm-engine/reading_cursor.h"

using namespace alarm_engine;

namespace {

std::string get_test_db() {
    const char* env = std::getenv("FORGESIGHT_TEST_DB");
    if (env)
        return env;
    return "dbname=forgesight_test";
}

void exec_and_clear(PGconn* conn, const char* sql) {
    auto* res = PQexec(conn, sql);
    PQclear(res);
}

std::int64_t insert_reading(PGconn* conn, const std::string& device, const std::string& sensor,
                            double value, const std::string& ts) {
    const auto value_str = std::to_string(value);
    const char* params[4] = {device.c_str(), sensor.c_str(), value_str.c_str(), ts.c_str()};
    int lengths[4] = {static_cast<int>(device.size()), static_cast<int>(sensor.size()),
                      static_cast<int>(value_str.size()), static_cast<int>(ts.size())};
    int formats[4] = {0, 0, 0, 0};
    auto* res =
        PQexecParams(conn,
                     "INSERT INTO readings (device_id, sensor, value, unit, timestamp, anomaly) "
                     "VALUES ($1, $2, $3, 'C', $4, false) RETURNING id",
                     4, nullptr, params, lengths, formats, 0);
    std::int64_t id = -1;
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
        id = std::stoll(PQgetvalue(res, 0, 0));
    PQclear(res);
    return id;
}

bool contains_id(const std::vector<PolledReading>& rows, std::int64_t id) {
    for (const auto& row : rows) {
        if (row.id == id)
            return true;
    }
    return false;
}

} // namespace

class ReadingCursorTest : public ::testing::Test {
  protected:
    PGconn* conn = nullptr;

    void SetUp() override {
        conn = PQconnectdb(get_test_db().c_str());
        if (PQstatus(conn) != CONNECTION_OK) {
            PQfinish(conn);
            conn = nullptr;
            GTEST_SKIP() << "Test DB not available";
        }
        exec_and_clear(conn, "CREATE TABLE IF NOT EXISTS readings ("
                             "  id BIGSERIAL PRIMARY KEY,"
                             "  device_id TEXT NOT NULL,"
                             "  sensor TEXT NOT NULL,"
                             "  value DOUBLE PRECISION NOT NULL,"
                             "  unit TEXT NOT NULL,"
                             "  timestamp TIMESTAMPTZ NOT NULL,"
                             "  anomaly BOOLEAN NOT NULL DEFAULT FALSE,"
                             "  created_at TIMESTAMPTZ NOT NULL DEFAULT NOW())");
    }

    void TearDown() override {
        if (!conn)
            return;
        exec_and_clear(conn, "DELETE FROM readings WHERE device_id = 'cursor-test'");
        PQfinish(conn);
    }
};

TEST_F(ReadingCursorTest, SameTimestampRowCommittedLaterIsNotSkipped) {
    const char* ts = "2099-03-01T00:00:00Z";
    const auto first = insert_reading(conn, "cursor-test", "temperature", 10.0, ts);
    const auto second = insert_reading(conn, "cursor-test", "pressure", 2.5, ts);
    ASSERT_GT(first, 0);
    ASSERT_GT(second, first);

    // The poll has already consumed `first`. A timestamp cursor at `ts` would
    // drop `second` because it is not strictly newer. The id cursor must not.
    auto after_first = fetch_readings_after(conn, first);
    EXPECT_FALSE(contains_id(after_first, first));
    ASSERT_TRUE(contains_id(after_first, second));

    auto after_second = fetch_readings_after(conn, second);
    EXPECT_FALSE(contains_id(after_second, first));
    EXPECT_FALSE(contains_id(after_second, second));
}

TEST_F(ReadingCursorTest, CatchUpSkipsAlreadyCommittedRows) {
    const auto caught_up = catch_up_reading_id(conn);
    ASSERT_TRUE(caught_up.has_value());
    const auto id = insert_reading(conn, "cursor-test", "vibration", 4.0, "2099-03-01T00:00:01Z");
    ASSERT_GT(id, *caught_up);

    auto pending = fetch_readings_after(conn, *caught_up);
    EXPECT_TRUE(contains_id(pending, id));

    const auto again = catch_up_reading_id(conn);
    ASSERT_TRUE(again.has_value());
    EXPECT_GE(*again, id);
    auto none = fetch_readings_after(conn, *again);
    EXPECT_FALSE(contains_id(none, id));
}

TEST(ReadingCursorNullConn, NullConnDoesNotReplayHistory) {
    EXPECT_FALSE(catch_up_reading_id(nullptr).has_value());
    EXPECT_TRUE(fetch_readings_after(nullptr, 0).empty());
}
