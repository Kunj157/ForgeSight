#pragma once

#include <QHttpServer>
#include <QWebSocket>
#include <QWebSocketServer>
#include <memory>

#include "api/device_service.h"
#include "api/rule_service.h"
#include "api/ws_broadcaster.h"

namespace api {

class ApiServer : public QObject {
    Q_OBJECT
public:
    ApiServer(void* conn, QObject* parent = nullptr);
    ~ApiServer() override;

    bool start(quint16 httpPort = 8080, quint16 wsPort = 8081);
    quint16 port() const { return port_; }
    quint16 wsPort() const { return wsPort_; }
    void stop();

    WebSocketBroadcaster* broadcaster() { return &broadcaster_; }

private:
    void setupRoutes();
    void on_new_websocket_connection();
    void* conn_;
    quint16 port_ = 0;
    quint16 wsPort_ = 0;
    QHttpServer server_;
    std::unique_ptr<QWebSocketServer> wsServer_;
    DeviceService deviceService_;
    RuleService ruleService_;
    WebSocketBroadcaster broadcaster_;
};

}  // namespace api
