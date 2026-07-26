#include "api/server.h"

#include <thread>

#include <nlohmann/json.hpp>
#include <crow.h>

namespace api {

struct Server::Impl {
    DeviceService& devices;
    RuleService& rules;
    WebSocketBroadcaster& broadcaster;
    crow::SimpleApp app;
    std::thread server_thread;

    Impl(DeviceService& d, RuleService& r, WebSocketBroadcaster& b)
        : devices(d), rules(r), broadcaster(b) {
        setup_routes();
    }

    void setup_routes() {
        auto& dev_svc = devices;
        auto& rule_svc = rules;

        CROW_ROUTE(app, "/api/devices")
            .methods("GET"_method)([&dev_svc]() {
                auto devs = dev_svc.list_devices();
                nlohmann::json arr = nlohmann::json::array();
                for (const auto& d : devs) {
                    arr.push_back({{"id", d.id}});
                }
                return crow::response(200, arr.dump());
            });

        CROW_ROUTE(app, "/api/devices/<string>/history")
            .methods("GET"_method)(
                [&dev_svc](const crow::request& req,
                           const std::string& device_id) {
                    std::string sensor;
                    std::string since;
                    auto* sv = req.url_params.get("sensor");
                    auto* sv2 = req.url_params.get("since");
                    if (sv) sensor = sv;
                    if (sv2) since = sv2;
                    auto readings =
                        dev_svc.get_history(device_id, sensor, since);
                    nlohmann::json arr = nlohmann::json::array();
                    for (const auto& r : readings) {
                        arr.push_back(
                            {{"device_id", r.device_id},
                             {"sensor", r.sensor},
                             {"value", r.value},
                             {"unit", r.unit},
                             {"timestamp", r.timestamp}});
                    }
                    return crow::response(200, arr.dump());
                });

        CROW_ROUTE(app, "/api/rules")
            .methods("GET"_method)([&rule_svc]() {
                auto rule_list = rule_svc.list_rules();
                nlohmann::json arr = nlohmann::json::array();
                for (const auto& r : rule_list) {
                    arr.push_back(
                        {{"id", r.id},
                         {"device_id", r.device_id},
                         {"sensor", r.sensor},
                         {"threshold", r.threshold},
                         {"severity",
                          alarm_engine::severity_to_string(r.severity)}});
                }
                return crow::response(200, arr.dump());
            });

        CROW_ROUTE(app, "/api/rules")
            .methods("POST"_method)(
                [&rule_svc](const crow::request& req) {
                    auto body =
                        nlohmann::json::parse(req.body, nullptr, false);
                    if (!body.is_object())
                        return crow::response(400, R"({"error":"invalid json"})");
                    alarm_engine::Rule r;
                    r.device_id = body.value("device_id", "");
                    r.sensor = body.value("sensor", "");
                    r.threshold = body.value("threshold", 0.0);
                    r.severity = alarm_engine::severity_from_string(
                        body.value("severity", "warning"));
                    r.condition = alarm_engine::Condition::GreaterThan;
                    auto id = rule_svc.create_rule(r);
                    if (id < 0)
                        return crow::response(500,
                                              R"({"error":"create failed"})");
                    nlohmann::json resp = {{"id", id}};
                    return crow::response(201, resp.dump());
                });

        CROW_ROUTE(app, "/api/rules/<int>")
            .methods("GET"_method)([&rule_svc](int id) {
                auto rule = rule_svc.get_rule(id);
                if (!rule.has_value())
                    return crow::response(404, R"({"error":"not found"})");
                nlohmann::json obj = {
                    {"id", rule->id},
                    {"device_id", rule->device_id},
                    {"sensor", rule->sensor},
                    {"threshold", rule->threshold},
                    {"severity",
                     alarm_engine::severity_to_string(rule->severity)}};
                return crow::response(200, obj.dump());
            });

        CROW_ROUTE(app, "/api/rules/<int>")
            .methods("DELETE"_method)([&rule_svc](int id) {
                bool ok = rule_svc.delete_rule(id);
                if (!ok)
                    return crow::response(404, R"({"error":"not found"})");
                return crow::response(200);
            });

        CROW_ROUTE(app, "/ws")
            .websocket(&app)
            .onaccept([&broadcaster = this->broadcaster](
                          const crow::request&, void**) {
                return true;
            })
            .onopen([&broadcaster = this->broadcaster](
                         crow::websocket::connection& conn) {
                auto id = broadcaster.add_connection(
                    [&conn](const std::string& msg) {
                        conn.send_text(msg);
                    });
                conn.userdata(new ConnectionId(id));
            })
            .onclose([&broadcaster = this->broadcaster](
                         crow::websocket::connection& conn,
                         const std::string&, uint16_t) {
                auto* id_ptr =
                    static_cast<ConnectionId*>(conn.userdata());
                if (id_ptr) {
                    broadcaster.remove_connection(*id_ptr);
                    delete id_ptr;
                }
            })
            .onmessage(
                [](crow::websocket::connection&, const std::string&,
                   bool) {});
    }
};

Server::Server(DeviceService& devices, RuleService& rules,
               WebSocketBroadcaster& broadcaster)
    : impl_(std::make_unique<Impl>(devices, rules, broadcaster)) {}

Server::~Server() {
    stop();
}

void Server::run(std::uint16_t port) {
    impl_->app.port(port);
    impl_->server_thread = std::thread([this, port]() {
        impl_->app.port(port).run();
    });
}

void Server::stop() {
    impl_->app.stop();
    if (impl_->server_thread.joinable()) {
        impl_->server_thread.join();
    }
}

}  // namespace api
