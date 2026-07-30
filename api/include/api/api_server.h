#pragma once

#include <QHttpServer>
#include <QTimer>
#include <QWebSocket>
#include <QWebSocketServer>
#include <memory>
#include <string>

#include <mqtt/async_client.h>

#include "api/device_service.h"
#include "api/rule_service.h"
#include "api/ws_broadcaster.h"

namespace api {

class ApiServer : public QObject {
    Q_OBJECT
public:
    ApiServer(void* conn, QObject* parent = nullptr);
    ~ApiServer() override;

    bool start(quint16 httpPort = 8080, quint16 wsPort = 0,
               const std::string& mqttBroker = "");
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
    void* conn_;
    quint16 port_ = 0;
    quint16 wsPort_ = 0;
    QHttpServer server_;
    std::unique_ptr<QWebSocketServer> wsServer_;
    std::unique_ptr<mqtt::async_client> mqtt_;
    DeviceService deviceService_;
    RuleService ruleService_;
    WebSocketBroadcaster broadcaster_;
    std::string last_readings_since_;
    std::string last_alarms_since_;
};

}  // namespace api
