#include <gtest/gtest.h>

#include <libpq-fe.h>
#include <nlohmann/json.hpp>

#include <asio.hpp>
#include <crow.h>
#include <crow/http_server.h>

#include "api/device_service.h"
#include "api/rule_service.h"
#include "api/ws_broadcaster.h"
#include "alarm-engine/types.h"

using json = nlohmann::json;

namespace {

std::string get_test_db() {
    const char* env = std::getenv("FORGESIGHT_TEST_DB");
    if (env) return env;
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

void seed_reading(void* conn, const std::string& device,
                  const std::string& sensor, double value,
                  const std::string& ts) {
    const char* params[4] = {device.c_str(), sensor.c_str(),
                              std::to_string(value).c_str(), ts.c_str()};
    int lengths[4] = {static_cast<int>(device.size()),
                      static_cast<int>(sensor.size()),
                      static_cast<int>(std::to_string(value).size()),
                      static_cast<int>(ts.size())};
    int formats[4] = {0, 0, 0, 0};
    auto* res = PQexecParams(
        static_cast<PGconn*>(conn),
        "INSERT INTO readings (device_id, sensor, value, unit, timestamp, anomaly) "
        "VALUES ($1, $2, $3, '\u00b0C', $4, false)",
        4, nullptr, params, lengths, formats, 0);
    PQclear(res);
}

void seed_rule(void* conn, const std::string& device,
               const std::string& sensor, double threshold) {
    const char* params[2] = {device.c_str(), sensor.c_str()};
    int lengths[2] = {static_cast<int>(device.size()),
                      static_cast<int>(sensor.size())};
    int formats[2] = {0, 0};
    auto* res = PQexecParams(
        static_cast<PGconn*>(conn),
        "INSERT INTO alarm_rules (device_id, sensor, condition, threshold, severity) "
        "VALUES ($1, $2, 'gt', $3, 'warning')",
        2, nullptr, params, lengths, formats, 0);
    PQclear(res);
}

void cleanup_test_data(void* conn, const std::string& prefix) {
    auto* pg = static_cast<PGconn*>(conn);
    PQexec(pg, ("DELETE FROM readings WHERE device_id = '" + prefix + "'").c_str());
    PQexec(pg, ("DELETE FROM alarm_rules WHERE device_id = '" + prefix + "'").c_str());
}

class HttpTestClient {
public:
    HttpTestClient(const std::string& host, uint16_t port)
        : host_(host), port_(port) {}

    struct Response {
        int code = 0;
        std::string body;
    };

    Response get(const std::string& path) {
        asio::io_context io;
        asio::ip::tcp::socket sock(io);
        sock.connect(
            asio::ip::tcp::endpoint(asio::ip::make_address(host_), port_));
        std::string req = "GET " + path + " HTTP/1.1\r\nHost: " + host_ +
                          "\r\nConnection: close\r\n\r\n";
        asio::write(sock, asio::buffer(req));
        return read_response(sock);
    }

    Response post(const std::string& path, const std::string& body) {
        asio::io_context io;
        asio::ip::tcp::socket sock(io);
        sock.connect(
            asio::ip::tcp::endpoint(asio::ip::make_address(host_), port_));
        std::string req = "POST " + path + " HTTP/1.1\r\nHost: " + host_ +
                          "\r\nContent-Type: application/json\r\nContent-Length: " +
                          std::to_string(body.size()) +
                          "\r\nConnection: close\r\n\r\n" + body;
        asio::write(sock, asio::buffer(req));
        return read_response(sock);
    }

    Response del(const std::string& path) {
        asio::io_context io;
        asio::ip::tcp::socket sock(io);
        sock.connect(
            asio::ip::tcp::endpoint(asio::ip::make_address(host_), port_));
        std::string req = "DELETE " + path + " HTTP/1.1\r\nHost: " + host_ +
                          "\r\nConnection: close\r\n\r\n";
        asio::write(sock, asio::buffer(req));
        return read_response(sock);
    }

private:
    Response read_response(asio::ip::tcp::socket& sock) {
        Response r;
        char buf[4096];
        asio::error_code ec;
        std::string data;
        while (size_t n = sock.read_some(asio::buffer(buf), ec)) {
            data.append(buf, n);
        }
        auto sep = data.find("\r\n\r\n");
        if (sep != std::string::npos) {
            std::string status = data.substr(0, data.find("\r\n"));
            auto sp = status.find(' ');
            if (sp != std::string::npos)
                r.code = std::stoi(status.substr(sp + 1));
            r.body = data.substr(sep + 4);
        }
        return r;
    }

    std::string host_;
    uint16_t port_;
};

}  // namespace

class ApiServerTest : public ::testing::Test {
protected:
    void* conn = nullptr;
    std::unique_ptr<api::DeviceService> devices;
    std::unique_ptr<api::RuleService> rules;
    std::unique_ptr<api::WebSocketBroadcaster> broadcaster;
    std::unique_ptr<crow::SimpleApp> app;
    std::thread server_thread;
    uint16_t port_ = 0;

    void SetUp() override {
        conn = connect_db();
        if (!conn) GTEST_SKIP() << "Test DB not available";
        devices = std::make_unique<api::DeviceService>(conn);
        rules = std::make_unique<api::RuleService>(conn);
        broadcaster = std::make_unique<api::WebSocketBroadcaster>();
        port_ = static_cast<uint16_t>(19000 + rand() % 1000);
    }

    void TearDown() override {
        if (app) app->stop();
        if (server_thread.joinable()) server_thread.join();
        app.reset();
        if (conn) {
            cleanup_test_data(conn, "api-server-test");
            PQfinish(static_cast<PGconn*>(conn));
        }
    }

    void start_server() {
        app = std::make_unique<crow::SimpleApp>();
        auto* dev_svc = devices.get();
        auto* rule_svc = rules.get();

        CROW_ROUTE((*app), "/api/devices")
            .methods("GET"_method)([dev_svc]() {
                auto devs = dev_svc->list_devices();
                json arr = json::array();
                for (const auto& d : devs) {
                    arr.push_back({{"id", d.id}});
                }
                return crow::response(200, arr.dump());
            });

        CROW_ROUTE((*app), "/api/devices/<string>/history")
            .methods("GET"_method)(
                [dev_svc](const crow::request& req,
                          const std::string& device_id) {
                    std::string sensor;
                    std::string since;
                    auto* sv = req.url_params.get("sensor");
                    auto* sv2 = req.url_params.get("since");
                    if (sv) sensor = sv;
                    if (sv2) since = sv2;
                    auto readings =
                        dev_svc->get_history(device_id, sensor, since);
                    json arr = json::array();
                    for (const auto& r : readings) {
                        arr.push_back({{"device_id", r.device_id},
                                       {"sensor", r.sensor},
                                       {"value", r.value},
                                       {"unit", r.unit},
                                       {"timestamp", r.timestamp}});
                    }
                    return crow::response(200, arr.dump());
                });

        CROW_ROUTE((*app), "/api/rules")
            .methods("GET"_method)([rule_svc]() {
                auto rule_list = rule_svc->list_rules();
                json arr = json::array();
                for (const auto& r : rule_list) {
                    arr.push_back({{"id", r.id},
                                   {"device_id", r.device_id},
                                   {"sensor", r.sensor},
                                   {"threshold", r.threshold},
                                   {"severity",
                                    alarm_engine::severity_to_string(
                                        r.severity)}});
                }
                return crow::response(200, arr.dump());
            });

        CROW_ROUTE((*app), "/api/rules")
            .methods("POST"_method)([rule_svc](const crow::request& req) {
                auto body = json::parse(req.body, nullptr, false);
                if (!body.is_object())
                    return crow::response(400, R"({"error":"invalid json"})");
                alarm_engine::Rule r;
                r.device_id = body.value("device_id", "");
                r.sensor = body.value("sensor", "");
                r.threshold = body.value("threshold", 0.0);
                r.severity = alarm_engine::severity_from_string(
                    body.value("severity", "warning"));
                r.condition = alarm_engine::Condition::GreaterThan;
                auto id = rule_svc->create_rule(r);
                if (id < 0)
                    return crow::response(500, R"({"error":"create failed"})");
                json resp = {{"id", id}};
                return crow::response(201, resp.dump());
            });

        CROW_ROUTE((*app), "/api/rules/<int>")
            .methods("GET"_method)([rule_svc](int id) {
                auto rule = rule_svc->get_rule(id);
                if (!rule.has_value())
                    return crow::response(404, R"({"error":"not found"})");
                json obj = {{"id", rule->id},
                            {"device_id", rule->device_id},
                            {"sensor", rule->sensor},
                            {"threshold", rule->threshold},
                            {"severity",
                             alarm_engine::severity_to_string(rule->severity)}};
                return crow::response(200, obj.dump());
            });

        CROW_ROUTE((*app), "/api/rules/<int>")
            .methods("DELETE"_method)([rule_svc](int id) {
                bool ok = rule_svc->delete_rule(id);
                if (!ok)
                    return crow::response(404, R"({"error":"not found"})");
                return crow::response(200);
            });

        server_thread = std::thread([this]() { app->port(port_).run(); });
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
};

TEST_F(ApiServerTest, GetDevicesReturnsJsonArray) {
    seed_reading(conn, "api-server-test", "temperature", 65.0,
                 "2026-07-26T10:00:00Z");
    start_server();
    HttpTestClient client("127.0.0.1", port_);
    auto res = client.get("/api/devices");
    EXPECT_EQ(res.code, 200);
    auto body = json::parse(res.body, nullptr, false);
    EXPECT_TRUE(body.is_array());
}

TEST_F(ApiServerTest, GetHistoryReturnsJsonArray) {
    seed_reading(conn, "api-server-test", "temperature", 65.0,
                 "2026-07-26T10:00:00Z");
    start_server();
    HttpTestClient client("127.0.0.1", port_);
    auto res = client.get(
        "/api/devices/api-server-test/history"
        "?sensor=temperature&since=2026-01-01T00:00:00Z");
    EXPECT_EQ(res.code, 200);
    auto body = json::parse(res.body, nullptr, false);
    EXPECT_TRUE(body.is_array());
}

TEST_F(ApiServerTest, GetRulesReturnsJsonArray) {
    start_server();
    HttpTestClient client("127.0.0.1", port_);
    auto res = client.get("/api/rules");
    EXPECT_EQ(res.code, 200);
    auto body = json::parse(res.body, nullptr, false);
    EXPECT_TRUE(body.is_array());
}

TEST_F(ApiServerTest, CreateRuleReturns201) {
    start_server();
    HttpTestClient client("127.0.0.1", port_);
    json payload = {{"device_id", "api-server-test"},
                    {"sensor", "temperature"},
                    {"threshold", 85.0},
                    {"severity", "warning"}};
    auto res = client.post("/api/rules", payload.dump());
    EXPECT_EQ(res.code, 201);
    auto body = json::parse(res.body, nullptr, false);
    EXPECT_TRUE(body.contains("id"));
    EXPECT_GT(body["id"].get<int64_t>(), 0);
}

TEST_F(ApiServerTest, CreateRuleInvalidJsonReturns400) {
    start_server();
    HttpTestClient client("127.0.0.1", port_);
    auto res = client.post("/api/rules", "not json");
    EXPECT_EQ(res.code, 400);
}

TEST_F(ApiServerTest, GetRuleReturnsJsonObject) {
    alarm_engine::Rule r;
    r.device_id = "api-server-test";
    r.sensor = "temperature";
    r.condition = alarm_engine::Condition::GreaterThan;
    r.threshold = 80.0;
    r.severity = alarm_engine::Severity::Warning;
    auto rule_id = rules->create_rule(r);
    ASSERT_GT(rule_id, 0);

    start_server();
    HttpTestClient client("127.0.0.1", port_);
    auto res = client.get("/api/rules/" + std::to_string(rule_id));
    EXPECT_EQ(res.code, 200);
    auto body = json::parse(res.body, nullptr, false);
    EXPECT_TRUE(body.is_object());
    EXPECT_EQ(body["device_id"].get<std::string>(), "api-server-test");
}

TEST_F(ApiServerTest, GetRuleNonexistentReturns404) {
    start_server();
    HttpTestClient client("127.0.0.1", port_);
    auto res = client.get("/api/rules/999999999");
    EXPECT_EQ(res.code, 404);
}

TEST_F(ApiServerTest, DeleteRuleReturns200) {
    alarm_engine::Rule r;
    r.device_id = "api-server-test";
    r.sensor = "temperature";
    r.condition = alarm_engine::Condition::GreaterThan;
    r.threshold = 90.0;
    r.severity = alarm_engine::Severity::Critical;
    auto rule_id = rules->create_rule(r);
    ASSERT_GT(rule_id, 0);

    start_server();
    HttpTestClient client("127.0.0.1", port_);
    auto res = client.del("/api/rules/" + std::to_string(rule_id));
    EXPECT_EQ(res.code, 200);
}

TEST_F(ApiServerTest, DeleteRuleNonexistentReturns404) {
    start_server();
    HttpTestClient client("127.0.0.1", port_);
    auto res = client.del("/api/rules/999999999");
    EXPECT_EQ(res.code, 404);
}
