#include <cstdlib>
#include <string>

#include <libpq-fe.h>
#include <spdlog/spdlog.h>

#include "api/device_service.h"
#include "api/rule_service.h"
#include "api/server.h"
#include "api/ws_broadcaster.h"

int main(int argc, char* argv[]) {
    spdlog::set_level(spdlog::level::info);

    std::string db_conn_str =
        std::getenv("FORGESIGHT_DB") ?
        std::getenv("FORGESIGHT_DB") :
        "dbname=forgesight host=localhost";

    auto* conn = PQconnectdb(db_conn_str.c_str());
    if (PQstatus(conn) != CONNECTION_OK) {
        spdlog::critical("Failed to connect to DB: {}",
                         PQerrorMessage(conn));
        PQfinish(conn);
        return 1;
    }
    spdlog::info("Connected to PostgreSQL");

    api::DeviceService devices(conn);
    api::RuleService rules(conn);
    api::WebSocketBroadcaster broadcaster;

    api::Server server(devices, rules, broadcaster);

    std::uint16_t port = 8080;
    if (argc > 1) {
        port = static_cast<std::uint16_t>(std::atoi(argv[1]));
    }

    spdlog::info("Starting API server on port {}", port);
    server.run(port);

    return 0;
}
