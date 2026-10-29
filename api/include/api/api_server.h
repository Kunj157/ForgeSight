#pragma once

#include <QHostAddress>
#include <QHttpServer>
#include <QTimer>
#include <QWebSocket>
#include <QWebSocketServer>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include <mqtt/async_client.h>

#include "api/device_service.h"
#include "api/rule_service.h"
#include "api/ws_broadcaster.h"

namespace api {

class MqttBridge;

class ApiServer : public QObject {
    Q_OBJECT
  public:
    ApiServer(void* conn, QObject* parent = nullptr);
    ~ApiServer() override;

    // `bindAddress` restricts which network interfaces the HTTP/WS servers
    // accept connections on (defaults to all interfaces for backward
    // compatibility). `apiKey`, when non-empty, requires every `/api/*`
    // request to carry a matching `X-Api-Key` header and every WebSocket
    // connection to carry a matching `api_key` query parameter; `/health`
    // always stays open for liveness checks. An empty key (the default)
    // disables auth entirely so local development needs no extra setup.
    bool start(quint16 httpPort = 8080, quint16 wsPort = 0, const std::string& mqttBroker = "",
               const QHostAddress& bindAddress = QHostAddress::Any, const std::string& apiKey = "");
    quint16 port() const { return port_; }
    quint16 wsPort() const { return wsPort_; }
    void stop();

    WebSocketBroadcaster* broadcaster() { return &broadcaster_; }
    void broadcast_reading(const ingestion::Reading& r);
    void broadcast_alarm(const alarm_engine::Alarm& a);

  private:
    void setupRoutes();
    void on_new_websocket_connection();
    void connect_mqtt(const std::string& broker);
    void seed_default_device_locations();
    bool is_authorized(const QHttpServerRequest& req) const;
    void* conn_;
    quint16 port_ = 0;
    quint16 wsPort_ = 0;
    QHostAddress bindAddress_ = QHostAddress::Any;
    std::string apiKey_;
    QHttpServer server_;
    std::unique_ptr<QWebSocketServer> wsServer_;
    std::unique_ptr<mqtt::async_client> mqtt_;
    std::unique_ptr<MqttBridge> mqttBridge_;
    DeviceService deviceService_;
    RuleService ruleService_;
    WebSocketBroadcaster broadcaster_;
    // Unset until the catch-up query succeeds. Id 0 is a real cursor (empty
    // table); it must not be used as the "not ready" sentinel.
    std::optional<std::int64_t> last_reading_id_;
    std::optional<std::int64_t> last_alarm_id_;
};

} // namespace api
