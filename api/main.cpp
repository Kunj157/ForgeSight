#include <QCommandLineParser>
#include <QCoreApplication>
#include <QHostAddress>

#include <libpq-fe.h>
#include <spdlog/spdlog.h>

#include "api/api_server.h"
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

    return app.exec();
}
