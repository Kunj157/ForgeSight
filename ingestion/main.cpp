#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <string>
#include <thread>

#include <mqtt/async_client.h>
#include <spdlog/spdlog.h>

#include "ingestion/db_writer.h"
#include "ingestion/event_bus.h"
#include "ingestion/reading.h"
#include "ingestion/reading_parser.h"
#include "ingestion/thread_pool.h"

using namespace ingestion;

namespace {

std::atomic<bool> g_running{true};

void signal_handler(int) {
    g_running.store(false);
}

struct AppConfig {
    std::string mqtt_broker = "tcp://localhost:1883";
    std::string mqtt_topic = "factory/devices/#";
    std::string mqtt_client_id = "forgesight-ingestion";
    std::string db_conn_string = "host=localhost dbname=forgesight user=postgres";
    std::size_t worker_threads = 4;
    std::size_t db_batch_size = 100;
};

AppConfig parse_args(int argc, char* argv[]) {
    AppConfig cfg;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--broker" && i + 1 < argc)
            cfg.mqtt_broker = argv[++i];
        else if (arg == "--topic" && i + 1 < argc)
            cfg.mqtt_topic = argv[++i];
        else if (arg == "--db" && i + 1 < argc)
            cfg.db_conn_string = argv[++i];
        else if (arg == "--workers" && i + 1 < argc)
            cfg.worker_threads = std::stoul(argv[++i]);
        else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: ingestion [options]\n"
                      << "  --broker URL      MQTT broker (default: tcp://localhost:1883)\n"
                      << "  --topic TOPIC     MQTT topic filter (default: factory/devices/#)\n"
                      << "  --db CONNSTR      PostgreSQL connection string\n"
                      << "  --workers N       Worker thread count (default: 4)\n";
            std::exit(0);
        }
    }
    return cfg;
}

class IngestionCallback : public virtual mqtt::callback {
  public:
    IngestionCallback(ThreadPool& pool, EventBus& bus, ReadingParser& parser, DbWriter& db)
        : pool_(pool), bus_(bus), parser_(parser), db_(db) {}

    void message_arrived(mqtt::const_message_ptr msg) override {
        auto payload = msg->to_string();
        auto result = parser_.parse(payload);
        if (std::holds_alternative<ParseError>(result)) {
            spdlog::warn("Malformed message: {}", std::get<ParseError>(result).reason);
            return;
        }

        auto reading = std::get<Reading>(std::move(result));
        pool_.submit([this, r = std::move(reading)]() mutable {
            db_.write(r);
            bus_.publish({std::move(r), std::chrono::system_clock::now()});
        });
    }

    void connection_lost(const std::string& cause) override {
        spdlog::error("MQTT connection lost: {}", cause);
    }

  private:
    ThreadPool& pool_;
    EventBus& bus_;
    ReadingParser& parser_;
    DbWriter& db_;
};

} // namespace

int main(int argc, char* argv[]) {
    spdlog::set_level(spdlog::level::info);
    auto cfg = parse_args(argc, argv);

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    spdlog::info("Ingestion service starting — broker={}, topic={}, workers={}", cfg.mqtt_broker,
                 cfg.mqtt_topic, cfg.worker_threads);

    ThreadPool pool(cfg.worker_threads);
    EventBus event_bus;
    ReadingParser parser;
    DbWriter db_writer({cfg.db_conn_string, cfg.db_batch_size});

    if (!db_writer.is_connected()) {
        spdlog::error("Cannot connect to database — aborting");
        return 1;
    }

    mqtt::async_client client(cfg.mqtt_broker, cfg.mqtt_client_id);

    IngestionCallback cb(pool, event_bus, parser, db_writer);
    client.set_callback(cb);

    mqtt::connect_options conn_opts;
    conn_opts.set_keep_alive_interval(30);
    conn_opts.set_automatic_reconnect(true);

    try {
        client.connect(conn_opts)->wait();
        spdlog::info("Connected to MQTT broker");

        client.subscribe(cfg.mqtt_topic, 1)->wait();
        spdlog::info("Subscribed to {}", cfg.mqtt_topic);

        while (g_running.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            // Flush partial batches so history/alarms see data promptly.
            auto n = db_writer.flush();
            if (n > 0) {
                spdlog::debug("Flushed {} buffered readings", n);
            }
        }
    } catch (const mqtt::exception& e) {
        spdlog::error("MQTT error: {}", e.what());
    }

    spdlog::info("Shutting down...");
    client.disconnect()->wait();
    pool.shutdown();
    spdlog::info("Ingestion service stopped");

    return 0;
}
