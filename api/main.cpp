#include <QCommandLineParser>
#include <QCoreApplication>
#include <QHostAddress>

#include <libpq-fe.h>
#include <spdlog/spdlog.h>

#include <memory>

#include "api/api_server.h"
#include "api/device_service.h"
#include "api/grpc_server.h"
#include "api/rule_service.h"
#include "api/ws_broadcaster.h"

namespace {

void* connect_db(const std::string& conn_str) {
    auto* conn = PQconnectdb(conn_str.c_str());
    if (PQstatus(conn) != CONNECTION_OK) {
        spdlog::error("DB connection failed: {}", PQerrorMessage(conn));
        PQfinish(conn);
        return nullptr;
    }
    spdlog::info("Connected to PostgreSQL");
    return conn;
}

} // namespace

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    app.setApplicationName("ForgeSight API");

    QCommandLineParser parser;
    parser.setApplicationDescription("ForgeSight REST/WebSocket API server");
    parser.addHelpOption();
    parser.addOption({{"p", "port"}, "HTTP listen port", "port", "8080"});
    parser.addOption({{"w", "ws-port"}, "WebSocket listen port", "port", "8081"});
    parser.addOption(
        {{"d", "database"}, "PostgreSQL connection string", "connstr", "dbname=forgesight"});
    parser.addOption({{"m", "mqtt"}, "MQTT broker URL", "url", "tcp://localhost:1883"});
    parser.addOption({{"b", "bind"},
                      "Interface to bind HTTP/WS servers to (e.g. 127.0.0.1 for loopback-only)",
                      "address",
                      "0.0.0.0"});
    parser.addOption({{"k", "api-key"},
                      "Require this value in the X-Api-Key header / WS api_key query param on "
                      "every /api/* and WebSocket request. Falls back to FORGESIGHT_API_KEY. "
                      "Empty disables auth (default; fine for local dev only).",
                      "key",
                      ""});
    parser.addOption({{"g", "grpc-port"},
                      "gRPC listen port (0 disables). Falls back to FORGESIGHT_GRPC_PORT. "
                      "Off by default so the live stack stays REST/WS-only.",
                      "port",
                      "0"});
    parser.process(app);

    quint16 httpPort = parser.value("port").toUShort();
    quint16 wsPort = parser.value("ws-port").toUShort();
    std::string conn_str = parser.value("database").toStdString();
    std::string mqtt_broker = parser.value("mqtt").toStdString();
    QHostAddress bindAddress(parser.value("bind"));
    if (bindAddress.isNull()) {
        spdlog::error("Invalid --bind address: {}", parser.value("bind").toStdString());
        return 1;
    }
    std::string apiKey = parser.value("api-key").toStdString();
    if (apiKey.empty()) {
        apiKey = qEnvironmentVariable("FORGESIGHT_API_KEY").toStdString();
    }
    if (apiKey.empty()) {
        spdlog::warn("ApiServer starting with no API key configured — /api/* routes are "
                     "unauthenticated. Set --api-key or FORGESIGHT_API_KEY for any non-local "
                     "deployment.");
    }
    quint16 grpcPort = parser.value("grpc-port").toUShort();
    if (grpcPort == 0) {
        const auto envPort = qEnvironmentVariable("FORGESIGHT_GRPC_PORT");
        if (!envPort.isEmpty())
            grpcPort = envPort.toUShort();
    }

    auto* conn = connect_db(conn_str);

    api::ApiServer server(conn);
    if (!server.start(httpPort, wsPort, mqtt_broker, bindAddress, apiKey)) {
        spdlog::error("Failed to start API server");
        if (conn)
            PQfinish(static_cast<PGconn*>(conn));
        return 1;
    }

    spdlog::info("REST API:  http://127.0.0.1:{}", server.port());
    spdlog::info("WebSocket: ws://127.0.0.1:{}", server.wsPort());

    // Own PGconn: libpq connections are not thread-safe, and the gRPC
    // completion queue runs on a different thread than Qt's HTTP event loop.
    std::unique_ptr<api::DeviceService> grpc_devices;
    std::unique_ptr<api::RuleService> grpc_rules;
    std::unique_ptr<api::GrpcServer> grpc;
    void* grpc_conn = nullptr;
    if (grpcPort > 0) {
        grpc_conn = connect_db(conn_str);
        if (!grpc_conn) {
            if (conn)
                PQfinish(static_cast<PGconn*>(conn));
            return 1;
        }
        grpc_devices = std::make_unique<api::DeviceService>(grpc_conn);
        grpc_rules = std::make_unique<api::RuleService>(grpc_conn);
        grpc = std::make_unique<api::GrpcServer>(*grpc_devices, *grpc_rules, apiKey);
        const std::string grpc_bind =
            parser.value("bind").toStdString() + ":" + std::to_string(grpcPort);
        if (!grpc->start(grpc_bind)) {
            spdlog::error("Failed to start gRPC server on {}", grpc_bind);
            PQfinish(static_cast<PGconn*>(grpc_conn));
            if (conn)
                PQfinish(static_cast<PGconn*>(conn));
            return 1;
        }
        spdlog::info("gRPC:      {}:{}", parser.value("bind").toStdString(), grpc->port());
    }

    return app.exec();
}
