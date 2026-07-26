#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <string>
#include <thread>

#include <spdlog/spdlog.h>

#include "alarm-engine/alarm_store.h"
#include "alarm-engine/rule_evaluator.h"
#include "alarm-engine/types.h"

#include "ingestion/reading.h"
#include "ingestion/event_bus.h"

using namespace alarm_engine;

namespace {

std::atomic<bool> g_running{true};
void signal_handler(int) { g_running.store(false); }

struct AppConfig {
    std::string db_conn_string = "host=localhost dbname=forgesight user=postgres";
};

AppConfig parse_args(int argc, char* argv[]) {
    AppConfig cfg;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--db" && i + 1 < argc) cfg.db_conn_string = argv[++i];
        else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: alarm-engine [options]\n"
                      << "  --db CONNSTR  PostgreSQL connection string\n";
            std::exit(0);
        }
    }
    return cfg;
}

}  // namespace

int main(int argc, char* argv[]) {
    spdlog::set_level(spdlog::level::info);
    auto cfg = parse_args(argc, argv);

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    AlarmStore store({cfg.db_conn_string});
    if (!store.is_connected()) {
        spdlog::error("Cannot connect to database");
        return 1;
    }
    store.create_tables();

    RuleEvaluator evaluator;
    auto rules = store.load_rules();
    spdlog::info("Alarm engine started with {} rules", rules.size());

    while (g_running.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    spdlog::info("Alarm engine stopped");
    return 0;
}
