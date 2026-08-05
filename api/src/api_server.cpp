#include "api/api_server.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>
#include <memory>

#include <nlohmann/json.hpp>

#include <spdlog/spdlog.h>

#include "alarm-engine/types.h"
#include "ingestion/reading_parser.h"

using json = nlohmann::json;

namespace api {

class MqttBridge : public mqtt::callback {
  public:
    MqttBridge(ApiServer* server) : server_(server) {}
    void message_arrived(mqtt::const_message_ptr msg) override {
        ingestion::ReadingParser parser;
        auto result = parser.parse(msg->to_string());
        if (std::holds_alternative<ingestion::Reading>(result)) {
            auto r = std::get<ingestion::Reading>(std::move(result));
            QMetaObject::invokeMethod(
                server_, [this, r = std::move(r)]() { server_->broadcast_reading(r); },
                Qt::QueuedConnection);
        }
    }
    void connection_lost(const std::string& cause) override {
        spdlog::warn("API MQTT connection lost: {}", cause);
    }

  private:
    ApiServer* server_;
};

ApiServer::ApiServer(void* conn, QObject* parent)
    : QObject(parent), conn_(conn), deviceService_(conn), ruleService_(conn) {}

ApiServer::~ApiServer() = default;

bool ApiServer::start(quint16 httpPort, quint16 wsPort, const std::string& mqttBroker,
                      const QHostAddress& bindAddress, const std::string& apiKey) {
    bindAddress_ = bindAddress;
    apiKey_ = apiKey;

    setupRoutes();

    port_ = server_.listen(bindAddress_, httpPort);
    if (port_ == 0) {
        spdlog::error("ApiServer: failed to listen on HTTP port {}", httpPort);
        return false;
    }
    spdlog::info("ApiServer: REST listening on port {}", port_);
    if (!apiKey_.empty()) {
        spdlog::info("ApiServer: API key auth enabled for /api/* routes");
    }

    wsServer_ =
        std::make_unique<QWebSocketServer>("ForgeSight WS", QWebSocketServer::NonSecureMode, this);

    if (!wsServer_->listen(bindAddress_, wsPort)) {
        spdlog::error("ApiServer: failed to listen on WS port {}", wsPort);
        return false;
    }

    connect(wsServer_.get(), &QWebSocketServer::newConnection, this,
            &ApiServer::on_new_websocket_connection);

    wsPort_ = wsServer_->serverPort();
    spdlog::info("ApiServer: WebSocket listening on port {}", wsPort_);

    connect_mqtt(mqttBroker);
    seed_default_device_locations();

    last_readings_since_ =
        QDateTime::currentDateTimeUtc().addSecs(-10).toString(Qt::ISODate).toStdString();
    last_alarms_since_ =
        QDateTime::currentDateTimeUtc().addSecs(-10).toString(Qt::ISODate).toStdString();

    auto* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this]() {
        auto readings = deviceService_.get_readings_since(last_readings_since_);
        if (!readings.empty()) {
            last_readings_since_ = readings.back().timestamp;
            for (const auto& r : readings) {
                broadcast_reading(r);
            }
        }

        auto alarms = ruleService_.get_alarms_since(last_alarms_since_);
        if (!alarms.empty()) {
            last_alarms_since_ = alarms.back().timestamp;
            for (const auto& a : alarms) {
                broadcast_alarm(a);
            }
        }
    });
    timer->start(2000);

    return true;
}

void ApiServer::stop() {
    if (mqtt_) {
        try {
            mqtt_->disconnect()->wait();
        } catch (...) {
        }
    }
    wsServer_.reset();
    server_.listen(QHostAddress::Any, 0);
    port_ = 0;
    wsPort_ = 0;
}

bool ApiServer::is_authorized(const QHttpServerRequest& req) const {
    if (apiKey_.empty()) {
        return true;
    }
    return req.value("X-Api-Key").toStdString() == apiKey_;
}

void ApiServer::seed_default_device_locations() {
    // Mirrors simulators/config.yaml. Non-destructive (won't override a
    // location that was already assigned via set_device_location), so this
    // is safe to run on every startup.
    static const std::pair<const char*, std::pair<const char*, const char*>> kKnownDevices[] = {
        {"pump-001", {"Plant A", "Floor 1"}},
        {"compressor-001", {"Plant A", "Floor 2"}},
    };
    for (const auto& [id, location] : kKnownDevices) {
        deviceService_.seed_default_location(id, location.first, location.second);
    }
}

void ApiServer::connect_mqtt(const std::string& broker) {
    try {
        mqtt_ = std::make_unique<mqtt::async_client>(broker, "forgesight-api");
        mqttBridge_ = std::make_unique<MqttBridge>(this);
        mqtt_->set_callback(*mqttBridge_);

        mqtt::connect_options connOpts;
        connOpts.set_keep_alive_interval(30);
        connOpts.set_automatic_reconnect(true);

        mqtt_->connect(connOpts)->wait();
        mqtt_->subscribe("factory/devices/#", 1)->wait();
        spdlog::info("ApiServer: subscribed to MQTT broker {}", broker);
    } catch (const mqtt::exception& e) {
        spdlog::warn("ApiServer: MQTT not available ({}). Using DB poll only.", e.what());
    }
}

void ApiServer::broadcast_alarm(const alarm_engine::Alarm& a) {
    json j = {{"id", a.id},
              {"rule_id", a.rule_id},
              {"device_id", a.device_id},
              {"sensor", a.sensor},
              {"value", a.value},
              {"severity", alarm_engine::severity_to_string(a.severity)},
              {"message", a.message},
              {"timestamp", a.timestamp},
              {"acknowledged", a.acknowledged}};
    broadcaster_.broadcast(j.dump());
}

void ApiServer::broadcast_reading(const ingestion::Reading& r) {
    json j = {{"device_id", r.device_id}, {"sensor", r.sensor},       {"value", r.value},
              {"unit", r.unit},           {"timestamp", r.timestamp}, {"anomaly", r.anomaly}};
    broadcaster_.broadcast(j.dump());
}

void ApiServer::on_new_websocket_connection() {
    while (wsServer_->hasPendingConnections()) {
        auto* socket = wsServer_->nextPendingConnection();
        if (!socket)
            continue;

        if (!apiKey_.empty()) {
            QUrlQuery query(socket->requestUrl().query());
            if (query.queryItemValue("api_key").toStdString() != apiKey_) {
                spdlog::warn("WS client rejected: missing/incorrect api_key");
                socket->close(QWebSocketProtocol::CloseCodeNormal, "unauthorized");
                socket->deleteLater();
                continue;
            }
        }

        auto id = broadcaster_.add_connection([ws = socket](const std::string& msg) {
            ws->sendTextMessage(QString::fromStdString(msg));
        });

        connect(socket, &QWebSocket::disconnected, this, [this, id, socket]() {
            broadcaster_.remove_connection(id);
            socket->deleteLater();
        });

        spdlog::info("WS client connected (total {})", broadcaster_.connection_count());
    }
}

void ApiServer::setupRoutes() {
    server_.route("/api/devices", [this](const QHttpServerRequest& req) {
        if (!is_authorized(req)) {
            return QHttpServerResponse(QHttpServerResponse::StatusCode::Unauthorized);
        }
        auto devices = deviceService_.list_devices();

        json j = json::array();
        for (const auto& d : devices) {
            j.push_back({{"id", d.id},
                         {"name", d.name},
                         {"sensor", d.sensor},
                         {"last_reading_time", d.last_reading_time},
                         {"last_value", d.last_value},
                         {"last_unit", d.last_unit},
                         {"anomaly", d.anomaly},
                         {"plant", d.plant},
                         {"floor", d.floor}});
        }

        return QHttpServerResponse("application/json", QByteArray::fromStdString(j.dump()));
    });

    server_.route("/api/readings/latest", [this](const QHttpServerRequest& req) {
        if (!is_authorized(req)) {
            return QHttpServerResponse(QHttpServerResponse::StatusCode::Unauthorized);
        }
        auto readings = deviceService_.list_latest_readings();
        json j = json::array();
        for (const auto& r : readings) {
            j.push_back({{"device_id", r.device_id},
                         {"sensor", r.sensor},
                         {"value", r.value},
                         {"unit", r.unit},
                         {"timestamp", r.timestamp},
                         {"anomaly", r.anomaly}});
        }
        return QHttpServerResponse("application/json", QByteArray::fromStdString(j.dump()));
    });

    server_.route("/api/alarms", [this](const QHttpServerRequest& req) {
        if (!is_authorized(req)) {
            return QHttpServerResponse(QHttpServerResponse::StatusCode::Unauthorized);
        }
        auto alarms = ruleService_.list_alarms(false);
        json j = json::array();
        for (const auto& a : alarms) {
            j.push_back({{"id", a.id},
                         {"rule_id", a.rule_id},
                         {"device_id", a.device_id},
                         {"sensor", a.sensor},
                         {"value", a.value},
                         {"severity", alarm_engine::severity_to_string(a.severity)},
                         {"message", a.message},
                         {"timestamp", a.timestamp},
                         {"acknowledged", a.acknowledged}});
        }
        return QHttpServerResponse("application/json", QByteArray::fromStdString(j.dump()));
    });

    server_.route("/api/history/<arg>/<arg>", [this](const QString& deviceId, const QString& sensor,
                                                     const QHttpServerRequest& req) {
        if (!is_authorized(req)) {
            return QHttpServerResponse(QHttpServerResponse::StatusCode::Unauthorized);
        }
        QUrlQuery query(req.url().query());
        QString since = query.queryItemValue("since");
        if (since.isEmpty()) {
            since = "1970-01-01T00:00:00Z";
        }

        auto readings = deviceService_.get_history(deviceId.toStdString(), sensor.toStdString(),
                                                   since.toStdString());

        json j = json::array();
        for (const auto& r : readings) {
            j.push_back({{"device_id", r.device_id},
                         {"sensor", r.sensor},
                         {"value", r.value},
                         {"unit", r.unit},
                         {"timestamp", r.timestamp},
                         {"anomaly", r.anomaly}});
        }

        return QHttpServerResponse("application/json", QByteArray::fromStdString(j.dump()));
    });

    server_.route(
        "/api/rules", QHttpServerRequest::Method::Get, [this](const QHttpServerRequest& req) {
            if (!is_authorized(req)) {
                return QHttpServerResponse(QHttpServerResponse::StatusCode::Unauthorized);
            }
            auto rules = ruleService_.list_rules();

            json j = json::array();
            for (const auto& r : rules) {
                j.push_back({{"id", r.id},
                             {"device_id", r.device_id},
                             {"sensor", r.sensor},
                             {"condition", alarm_engine::condition_to_string(r.condition)},
                             {"threshold", r.threshold},
                             {"severity", alarm_engine::severity_to_string(r.severity)}});
            }

            return QHttpServerResponse("application/json", QByteArray::fromStdString(j.dump()));
        });

    server_.route(
        "/api/rules", QHttpServerRequest::Method::Post, [this](const QHttpServerRequest& req) {
            if (!is_authorized(req)) {
                return QHttpServerResponse(QHttpServerResponse::StatusCode::Unauthorized);
            }
            auto body = QJsonDocument::fromJson(req.body()).object();
            alarm_engine::Rule r;
            r.device_id = body.value("device_id").toString().toStdString();
            r.sensor = body.value("sensor").toString().toStdString();
            r.condition = alarm_engine::condition_from_string(
                body.value("condition").toString().toStdString());
            r.threshold = body.value("threshold").toDouble();
            r.severity =
                alarm_engine::severity_from_string(body.value("severity").toString().toStdString());

            auto id = ruleService_.create_rule(r);
            if (id < 0) {
                return QHttpServerResponse("application/json", QByteArrayLiteral("{\"ok\":false}"),
                                           QHttpServerResponse::StatusCode::BadRequest);
            }

            json j = {{"id", id}, {"ok", true}};
            return QHttpServerResponse("application/json", QByteArray::fromStdString(j.dump()),
                                       QHttpServerResponse::StatusCode::Created);
        });

    server_.route("/api/rules/<arg>", QHttpServerRequest::Method::Get,
                  [this](const QString& ruleId, const QHttpServerRequest& req) {
                      if (!is_authorized(req)) {
                          return QHttpServerResponse(QHttpServerResponse::StatusCode::Unauthorized);
                      }
                      bool parsed = false;
                      auto id = ruleId.toLongLong(&parsed);
                      if (!parsed) {
                          return QHttpServerResponse(QHttpServerResponse::StatusCode::BadRequest);
                      }
                      auto rule = ruleService_.get_rule(id);
                      if (!rule) {
                          return QHttpServerResponse(QHttpServerResponse::StatusCode::NotFound);
                      }
                      json j = {{"id", rule->id},
                                {"device_id", rule->device_id},
                                {"sensor", rule->sensor},
                                {"condition", alarm_engine::condition_to_string(rule->condition)},
                                {"threshold", rule->threshold},
                                {"severity", alarm_engine::severity_to_string(rule->severity)}};
                      return QHttpServerResponse("application/json",
                                                 QByteArray::fromStdString(j.dump()));
                  });

    server_.route(
        "/api/rules/<arg>", QHttpServerRequest::Method::Put,
        [this](const QString& ruleId, const QHttpServerRequest& req) {
            if (!is_authorized(req)) {
                return QHttpServerResponse(QHttpServerResponse::StatusCode::Unauthorized);
            }
            bool parsed = false;
            auto id = ruleId.toLongLong(&parsed);
            if (!parsed) {
                return QHttpServerResponse(QHttpServerResponse::StatusCode::BadRequest);
            }
            auto body = QJsonDocument::fromJson(req.body()).object();
            alarm_engine::Rule r;
            r.id = id;
            r.device_id = body.value("device_id").toString().toStdString();
            r.sensor = body.value("sensor").toString().toStdString();
            r.condition = alarm_engine::condition_from_string(
                body.value("condition").toString().toStdString());
            r.threshold = body.value("threshold").toDouble();
            r.severity =
                alarm_engine::severity_from_string(body.value("severity").toString().toStdString());

            bool ok = ruleService_.update_rule(r);
            json j = {{"ok", ok}};
            return QHttpServerResponse("application/json", QByteArray::fromStdString(j.dump()),
                                       ok ? QHttpServerResponse::StatusCode::Ok
                                          : QHttpServerResponse::StatusCode::NotFound);
        });

    server_.route("/api/rules/<arg>", QHttpServerRequest::Method::Delete,
                  [this](const QString& ruleId, const QHttpServerRequest& req) {
                      if (!is_authorized(req)) {
                          return QHttpServerResponse(QHttpServerResponse::StatusCode::Unauthorized);
                      }
                      bool parsed = false;
                      auto id = ruleId.toLongLong(&parsed);
                      if (!parsed) {
                          return QHttpServerResponse(QHttpServerResponse::StatusCode::BadRequest);
                      }
                      bool ok = ruleService_.delete_rule(id);
                      json j = {{"ok", ok}};
                      return QHttpServerResponse("application/json",
                                                 QByteArray::fromStdString(j.dump()),
                                                 ok ? QHttpServerResponse::StatusCode::Ok
                                                    : QHttpServerResponse::StatusCode::NotFound);
                  });

    server_.route("/api/alarms/<arg>/ack", QHttpServerRequest::Method::Post,
                  [this](const QString& alarmId, const QHttpServerRequest& req) {
                      if (!is_authorized(req)) {
                          return QHttpServerResponse(QHttpServerResponse::StatusCode::Unauthorized);
                      }
                      bool ok = false;
                      bool parsed = false;
                      std::int64_t id = alarmId.toLongLong(&parsed);
                      if (parsed) {
                          ok = ruleService_.acknowledge_alarm(id);
                      }

                      json j = {{"ok", ok}};
                      return QHttpServerResponse("application/json",
                                                 QByteArray::fromStdString(j.dump()),
                                                 ok ? QHttpServerResponse::StatusCode::Ok
                                                    : QHttpServerResponse::StatusCode::NotFound);
                  });

    server_.route("/health", [](const QHttpServerRequest& req) {
        (void)req;
        return QHttpServerResponse("application/json", QByteArrayLiteral("{\"status\":\"ok\"}"));
    });
}
} // namespace api
