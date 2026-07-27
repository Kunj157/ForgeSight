#include "api/api_server.h"

#include <QUrlQuery>

#include <nlohmann/json.hpp>

#include <spdlog/spdlog.h>

using json = nlohmann::json;

namespace api {

ApiServer::ApiServer(void* conn, QObject* parent)
    : QObject(parent), conn_(conn), deviceService_(conn), ruleService_(conn) {}

ApiServer::~ApiServer() = default;

bool ApiServer::start(quint16 httpPort, quint16 wsPort) {
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
    return true;
}

void ApiServer::stop() {
    wsServer_.reset();
    server_.listen(QHostAddress::Any, 0);
    port_ = 0;
    wsPort_ = 0;
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
