#include <gtest/gtest.h>

#include <cstdint>
#include <grpcpp/grpcpp.h>
#include <libpq-fe.h>

#include "alarm-engine/types.h"
#include "api/device_service.h"
#include "api/grpc_server.h"
#include "api/rule_service.h"
#include "forgesight.grpc.pb.h"
#include "forgesight.pb.h"

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

void exec_and_clear(void* conn, const char* sql) {
    auto* res = PQexec(static_cast<PGconn*>(conn), sql);
    PQclear(res);
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
                     "VALUES ($1, $2, $3, 'C', $4, false)",
                     4, nullptr, params, lengths, formats, 0);
    PQclear(res);
}

} // namespace

class GrpcServerTest : public ::testing::Test {
  protected:
    void* conn = nullptr;

    void SetUp() override {
        conn = connect_db();
        if (!conn)
            GTEST_SKIP() << "Test DB not available";
        exec_and_clear(conn,
                       "CREATE TABLE IF NOT EXISTS readings ("
                       "  id BIGSERIAL PRIMARY KEY, device_id TEXT NOT NULL, sensor TEXT NOT NULL,"
                       "  value DOUBLE PRECISION NOT NULL, unit TEXT NOT NULL,"
                       "  timestamp TIMESTAMPTZ NOT NULL, anomaly BOOLEAN NOT NULL DEFAULT FALSE,"
                       "  created_at TIMESTAMPTZ NOT NULL DEFAULT NOW())");
        exec_and_clear(conn,
                       "CREATE TABLE IF NOT EXISTS alarm_rules ("
                       "  id BIGSERIAL PRIMARY KEY, device_id TEXT NOT NULL, sensor TEXT NOT NULL,"
                       "  condition TEXT NOT NULL, threshold DOUBLE PRECISION NOT NULL,"
                       "  severity TEXT NOT NULL DEFAULT 'warning',"
                       "  message_template TEXT NOT NULL DEFAULT '')");
        exec_and_clear(conn, "CREATE TABLE IF NOT EXISTS alarms ("
                             "  id BIGSERIAL PRIMARY KEY,"
                             "  rule_id BIGINT REFERENCES alarm_rules(id),"
                             "  device_id TEXT NOT NULL, sensor TEXT NOT NULL,"
                             "  value DOUBLE PRECISION NOT NULL,"
                             "  severity TEXT NOT NULL DEFAULT 'warning',"
                             "  message TEXT NOT NULL DEFAULT '',"
                             "  timestamp TIMESTAMPTZ NOT NULL DEFAULT NOW(),"
                             "  acknowledged BOOLEAN NOT NULL DEFAULT FALSE,"
                             "  acknowledged_at TIMESTAMPTZ,"
                             "  created_at TIMESTAMPTZ NOT NULL DEFAULT NOW())");
    }

    void TearDown() override {
        if (conn) {
            exec_and_clear(conn, "DELETE FROM alarms WHERE device_id = 'grpc-test'");
            exec_and_clear(conn, "DELETE FROM alarm_rules WHERE device_id = 'grpc-test'");
            exec_and_clear(conn, "DELETE FROM readings WHERE device_id = 'grpc-test'");
            PQfinish(static_cast<PGconn*>(conn));
        }
    }
};

TEST_F(GrpcServerTest, ListDevicesAndHistory) {
    seed_reading(conn, "grpc-test", "temperature", 66.5, "2026-09-22T08:00:00Z");
    api::DeviceService devices(conn);
    api::RuleService rules(conn);
    api::GrpcServer server(devices, rules);
    ASSERT_TRUE(server.start("127.0.0.1:0"));
    ASSERT_GT(server.port(), 0);

    auto stub = ::forgesight::ForgeSight::NewStub(grpc::CreateChannel(
        "127.0.0.1:" + std::to_string(server.port()), grpc::InsecureChannelCredentials()));

    grpc::ClientContext ctx;
    ::forgesight::Empty empty;
    ::forgesight::DeviceList list;
    auto status = stub->ListDevices(&ctx, empty, &list);
    ASSERT_TRUE(status.ok()) << status.error_message();

    bool found = false;
    for (const auto& d : list.devices()) {
        if (d.id() == "grpc-test") {
            found = true;
            EXPECT_DOUBLE_EQ(d.last_value(), 66.5);
        }
    }
    EXPECT_TRUE(found);

    grpc::ClientContext hctx;
    ::forgesight::HistoryRequest req;
    req.set_device_id("grpc-test");
    req.set_sensor("temperature");
    req.set_since("2026-09-22T00:00:00Z");
    ::forgesight::ReadingList hist;
    status = stub->GetHistory(&hctx, req, &hist);
    ASSERT_TRUE(status.ok()) << status.error_message();
    ASSERT_GE(hist.readings_size(), 1);
    EXPECT_EQ(hist.readings(0).device_id(), "grpc-test");

    server.stop();
}

TEST_F(GrpcServerTest, RejectsMissingApiKey) {
    api::DeviceService devices(conn);
    api::RuleService rules(conn);
    api::GrpcServer server(devices, rules, "secret");
    ASSERT_TRUE(server.start("127.0.0.1:0"));

    auto stub = ::forgesight::ForgeSight::NewStub(grpc::CreateChannel(
        "127.0.0.1:" + std::to_string(server.port()), grpc::InsecureChannelCredentials()));

    grpc::ClientContext ctx;
    ::forgesight::Empty empty;
    ::forgesight::DeviceList list;
    auto status = stub->ListDevices(&ctx, empty, &list);
    EXPECT_EQ(status.error_code(), grpc::StatusCode::UNAUTHENTICATED);

    grpc::ClientContext authed;
    authed.AddMetadata("x-api-key", "secret");
    status = stub->ListDevices(&authed, empty, &list);
    EXPECT_TRUE(status.ok()) << status.error_message();

    server.stop();
}

TEST_F(GrpcServerTest, ListAlarmsAndAcknowledge) {
    api::DeviceService devices(conn);
    api::RuleService rules(conn);
    alarm_engine::Rule r;
    r.device_id = "grpc-test";
    r.sensor = "temperature";
    r.condition = alarm_engine::Condition::GreaterThan;
    r.threshold = 80.0;
    r.severity = alarm_engine::Severity::Warning;
    auto rule_id = rules.create_rule(r);
    ASSERT_GT(rule_id, 0);

    std::string rid = std::to_string(rule_id);
    const char* params[1] = {rid.c_str()};
    int lengths[1] = {static_cast<int>(rid.size())};
    int formats[1] = {0};
    auto* res = PQexecParams(
        static_cast<PGconn*>(conn),
        "INSERT INTO alarms (rule_id, device_id, sensor, value, severity, message, timestamp) "
        "VALUES ($1, 'grpc-test', 'temperature', 91.0, 'warning', 'hot', '2026-09-22T08:00:00Z')",
        1, nullptr, params, lengths, formats, 0);
    ASSERT_EQ(PQresultStatus(res), PGRES_COMMAND_OK) << PQerrorMessage(static_cast<PGconn*>(conn));
    PQclear(res);

    api::GrpcServer server(devices, rules);
    ASSERT_TRUE(server.start("127.0.0.1:0"));

    auto stub = ::forgesight::ForgeSight::NewStub(grpc::CreateChannel(
        "127.0.0.1:" + std::to_string(server.port()), grpc::InsecureChannelCredentials()));

    grpc::ClientContext ctx;
    ::forgesight::AlarmQuery q;
    q.set_unacknowledged_only(true);
    ::forgesight::AlarmList list;
    auto status = stub->ListAlarms(&ctx, q, &list);
    ASSERT_TRUE(status.ok()) << status.error_message();

    std::int64_t found_id = 0;
    for (const auto& a : list.alarms()) {
        if (a.device_id() == "grpc-test") {
            found_id = a.id();
            EXPECT_DOUBLE_EQ(a.value(), 91.0);
            EXPECT_FALSE(a.acknowledged());
        }
    }
    ASSERT_GT(found_id, 0);

    grpc::ClientContext actx;
    ::forgesight::AckRequest ack;
    ack.set_id(found_id);
    ::forgesight::AckReply reply;
    status = stub->AcknowledgeAlarm(&actx, ack, &reply);
    ASSERT_TRUE(status.ok()) << status.error_message();
    EXPECT_TRUE(reply.ok());

    grpc::ClientContext ctx2;
    ::forgesight::AlarmList after;
    status = stub->ListAlarms(&ctx2, q, &after);
    ASSERT_TRUE(status.ok()) << status.error_message();
    for (const auto& a : after.alarms())
        EXPECT_NE(a.id(), found_id);

    server.stop();
}
