#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include <libpq-fe.h>
#include <spdlog/spdlog.h>

#include "alarm-engine/alarm_store.h"
#include "alarm-engine/rule_evaluator.h"
#include "alarm-engine/types.h"

#include "ingestion/reading.h"

using namespace alarm_engine;

namespace {

std::atomic<bool> g_running{true};
void signal_handler(int) {
    g_running.store(false);
}

struct AppConfig {
    std::string db_conn_string = "host=localhost dbname=forgesight user=postgres";
    bool seed_rules = false;
    int poll_interval_ms = 2000;
};

AppConfig parse_args(int argc, char* argv[]) {
    AppConfig cfg;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--db" && i + 1 < argc)
            cfg.db_conn_string = argv[++i];
        else if (arg == "--seed")
            cfg.seed_rules = true;
        else if (arg == "--poll-ms" && i + 1 < argc)
            cfg.poll_interval_ms = std::stoi(argv[++i]);
        else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: alarm-engine [options]\n"
                      << "  --db CONNSTR    PostgreSQL connection string\n"
                      << "  --seed           Insert default rules if empty\n"
                      << "  --poll-ms N      DB poll interval (default 2000)\n";
            std::exit(0);
        }
    }
    return cfg;
}

void seed_default_rules(AlarmStore& store) {
    auto existing = store.load_rules();
    if (!existing.empty())
        return;

    Rule rules[] = {
        {0, "pump-001", "temperature", Condition::GreaterThan, 80.0, Severity::Warning,
         "{device} {sensor} ({value}) exceeded threshold {threshold}"},
        {0, "pump-001", "temperature", Condition::GreaterThan, 95.0, Severity::Critical,
         "{device} {sensor} CRITICAL ({value}) exceeded {threshold}"},
        {0, "pump-001", "pressure", Condition::LessThan, 1.0, Severity::Critical,
         "{device} pressure critically low ({value} < {threshold})"},
        {0, "pump-001", "pressure", Condition::GreaterThan, 4.5, Severity::Warning,
         "{device} pressure high ({value} > {threshold})"},
        {0, "pump-001", "vibration", Condition::GreaterThan, 10.0, Severity::Warning,
         "{device} vibration high ({value} > {threshold})"},
        {0, "compressor-001", "temperature", Condition::GreaterThan, 90.0, Severity::Warning,
         "{device} {sensor} ({value}) exceeded {threshold}"},
        {0, "compressor-001", "temperature", Condition::GreaterThan, 105.0, Severity::Critical,
         "{device} {sensor} CRITICAL ({value}) exceeded {threshold}"},
        {0, "compressor-001", "current", Condition::GreaterThan, 25.0, Severity::Warning,
         "{device} current high ({value} > {threshold})"},
    };

    int count = 0;
    for (const auto& r : rules) {
        if (store.add_rule(r))
            ++count;
    }
    spdlog::info("Seeded {} default rules", count);
}

std::vector<ingestion::Reading> fetch_readings_since(PGconn* conn, const std::string& since) {

    std::vector<ingestion::Reading> readings;
    const char* params[1] = {since.c_str()};
    int lens[1] = {static_cast<int>(since.size())};
    int fmts[1] = {0};

    auto* res = PQexecParams(conn,
                             "SELECT device_id, sensor, value, unit, timestamp::text, anomaly "
                             "FROM readings WHERE timestamp > $1 ORDER BY timestamp",
                             1, nullptr, params, lens, fmts, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return readings;
    }

    int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i) {
        ingestion::Reading r;
        r.device_id = PQgetvalue(res, i, 0);
        r.sensor = PQgetvalue(res, i, 1);
        r.value = std::stod(PQgetvalue(res, i, 2));
        r.unit = PQgetvalue(res, i, 3);
        r.timestamp = PQgetvalue(res, i, 4);
        r.anomaly = (PQgetvalue(res, i, 5)[0] == 't');
        readings.push_back(std::move(r));
    }

    PQclear(res);
    return readings;
}

} // namespace

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

    if (cfg.seed_rules) {
        seed_default_rules(store);
    }

    auto* conn = PQconnectdb(cfg.db_conn_string.c_str());
    if (PQstatus(conn) != CONNECTION_OK) {
        spdlog::error("Cannot open read-only DB connection");
        PQfinish(conn);
        return 1;
    }

    RuleEvaluator evaluator;
    auto rules = store.load_rules();
    spdlog::info("Alarm engine started with {} rules (poll every {}ms)", rules.size(),
                 cfg.poll_interval_ms);

    std::string last_check;
    {
        auto* res = PQexec(conn, "SELECT COALESCE(MAX(timestamp)::text, '1970-01-01T00:00:00Z') "
                                 "FROM readings");
        if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0) {
            last_check = PQgetvalue(res, 0, 0);
        }
        PQclear(res);
    }

    while (g_running.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(cfg.poll_interval_ms));
        if (!g_running.load())
            break;

        auto readings = fetch_readings_since(conn, last_check);
        if (readings.empty())
            continue;

        int alarm_count = 0;
        for (const auto& r : readings) {
            auto alarms =
                evaluator.evaluate_all(rules, r.device_id, r.sensor, r.value, r.timestamp);
            for (const auto& a : alarms) {
                if (store.write_alarm(a))
                    ++alarm_count;
            }
            last_check = r.timestamp;
        }

        if (alarm_count > 0) {
            spdlog::info("Evaluated {} readings, triggered {} alarms", readings.size(),
                         alarm_count);
        }
    }

    PQfinish(conn);
    spdlog::info("Alarm engine stopped");
    return 0;
}
