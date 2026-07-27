#include "api/api_server.h"

#include <QDateTime>
#include <QUrlQuery>
#include <memory>

#include <nlohmann/json.hpp>

#include <spdlog/spdlog.h>

#include "ingestion/reading_parser.h"

using json = nlohmann::json;

namespace api {

namespace {

class MqttBridge : public mqtt::callback {
public:
    MqttBridge(ApiServer* server) : server_(server) {}
    void message_arrived(mqtt::const_message_ptr msg) override {
        ingestion::ReadingParser parser;
        auto result = parser.parse(msg->to_string());
        if (std::holds_alternative<ingestion::Reading>(result)) {
            auto r = std::get<ingestion::Reading>(std::move(result));
            QMetaObject::invokeMethod(server_, [this, r = std::move(r)]() {
                server_->broadcast_reading(r);
            }, Qt::QueuedConnection);
        }
    }
    void connection_lost(const std::string& cause) override {
        spdlog::warn("API MQTT connection lost: {}", cause);
    }
private:
    ApiServer* server_;
};

}  // namespace

ApiServer::ApiServer(void* conn, QObject* parent)
    : QObject(parent), conn_(conn), deviceService_(conn), ruleService_(conn) {}

ApiServer::~ApiServer() = default;

bool ApiServer::start(quint16 httpPort, quint16 wsPort,
                      const std::string& mqttBroker) {
    setupRoutes();

    port_ = server_.listen(QHostAddress::Any, httpPort);
    if (port_ == 0) {
        spdlog::error("ApiServer: failed to listen on HTTP port {}", httpPort);
        return false;
    }
    spdlog::info("ApiServer: REST listening on port {}", port_);

    wsServer_ = std::make_unique<QWebSocketServer>(
        "ForgeSight WS", QWebSocketServer::NonSecureMode, this);

    if (!wsServer_->listen(QHostAddress::Any, wsPort)) {
        spdlog::error("ApiServer: failed to listen on WS port {}", wsPort);
        return false;
    }

    connect(wsServer_.get(), &QWebSocketServer::newConnection,
            this, &ApiServer::on_new_websocket_connection);

    wsPort_ = wsServer_->serverPort();
    spdlog::info("ApiServer: WebSocket listening on port {}", wsPort_);

    connect_mqtt(mqttBroker);

    last_readings_since_ = QDateTime::currentDateTimeUtc()
        .addSecs(-10).toString(Qt::ISODate).toStdString();

    auto* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this]() {
        auto readings = deviceService_.get_readings_since(last_readings_since_);
        if (!readings.empty()) {
            last_readings_since_ = readings.back().timestamp;
            for (const auto& r : readings) {
                broadcast_reading(r);
            }
        }
    });
    timer->start(2000);

    return true;
}

void ApiServer::stop() {
    if (mqtt_) {
        try { mqtt_->disconnect()->wait(); } catch (...) {}
    }
    wsServer_.reset();
    server_.listen(QHostAddress::Any, 0);
    port_ = 0;
    wsPort_ = 0;
}

void ApiServer::connect_mqtt(const std::string& broker) {
    try {
        mqtt_ = std::make_unique<mqtt::async_client>(broker, "forgesight-api");
        auto* bridge = new MqttBridge(this);
        mqtt_->set_callback(*bridge);

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

void ApiServer::broadcast_reading(const ingestion::Reading& r) {
    json j = {
        {"device_id", r.device_id},
        {"sensor", r.sensor},
        {"value", r.value},
        {"unit", r.unit},
        {"timestamp", r.timestamp},
        {"anomaly", r.anomaly}
    };
    broadcaster_.broadcast(j.dump());
}

void ApiServer::on_new_websocket_connection() {
    while (wsServer_->hasPendingConnections()) {
        auto* socket = wsServer_->nextPendingConnection();
        if (!socket) continue;

        auto id = broadcaster_.add_connection(
            [ws = socket](const std::string& msg) {
                ws->sendTextMessage(QString::fromStdString(msg));
            });

        connect(socket, &QWebSocket::disconnected, this,
                [this, id, socket]() {
                    broadcaster_.remove_connection(id);
                    socket->deleteLater();
                });

        spdlog::info("WS client connected (total {})", broadcaster_.connection_count());
    }
}

void ApiServer::setupRoutes() {
    server_.route("/api/devices", [this](const QHttpServerRequest& req) {
        (void)req;
        auto devices = deviceService_.list_devices();

        json j = json::array();
        for (const auto& d : devices) {
            j.push_back({
                {"id", d.id},
                {"name", d.name},
                {"last_reading_time", d.last_reading_time},
                {"last_value", d.last_value},
                {"last_unit", d.last_unit}
            });
        }

        return QHttpServerResponse("application/json",
            QByteArray::fromStdString(j.dump()));
    });

    server_.route("/api/history/<arg>/<arg>",
        [this](const QString& deviceId, const QString& sensor,
               const QHttpServerRequest& req) {
            QUrlQuery query(req.url().query());
            QString since = query.queryItemValue("since");
            if (since.isEmpty()) {
                since = "1970-01-01T00:00:00Z";
            }

            auto readings = deviceService_.get_history(
                deviceId.toStdString(), sensor.toStdString(),
                since.toStdString());

            json j = json::array();
            for (const auto& r : readings) {
                j.push_back({
                    {"device_id", r.device_id},
                    {"sensor", r.sensor},
                    {"value", r.value},
                    {"unit", r.unit},
                    {"timestamp", r.timestamp},
                    {"anomaly", r.anomaly}
                });
            }

            return QHttpServerResponse("application/json",
                QByteArray::fromStdString(j.dump()));
        });

    server_.route("/api/rules",
        [this](const QHttpServerRequest& req) {
            (void)req;
            auto rules = ruleService_.list_rules();

            json j = json::array();
            for (const auto& r : rules) {
                j.push_back({
                    {"id", r.id},
                    {"device_id", r.device_id},
                    {"sensor", r.sensor},
                    {"condition", alarm_engine::condition_to_string(r.condition)},
                    {"threshold", r.threshold},
                    {"severity", alarm_engine::severity_to_string(r.severity)}
                });
            }

            return QHttpServerResponse("application/json",
                QByteArray::fromStdString(j.dump()));
        });
}

}  // namespace api
