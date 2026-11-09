#pragma once

#include <alarm-engine/rule_evaluator.h>
#include <alarm-engine/types.h>

#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace ui::demo {

struct SensorSpec {
    std::string name;
    std::string unit;
    double min = 0.0;
    double max = 0.0;
    double normal_mean = 0.0;
    double normal_std = 0.0;
};

struct DeviceSpec {
    std::string id;
    std::string name;
    std::string plant;
    std::string floor;
    std::vector<SensorSpec> sensors;
};

struct Reading {
    std::string device_id;
    std::string sensor;
    std::string unit;
    std::string timestamp; // ISO-8601 UTC, "YYYY-MM-DDTHH:MM:SSZ"
    double value = 0.0;
    bool anomaly = false;
};

struct TickResult {
    std::vector<Reading> readings;
    std::vector<alarm_engine::Alarm> new_alarms;
};

/// Simulated plant for the browser demo. Ported from simulators/config.yaml
/// and simulators/generator.py; alarms come from the real
/// alarm_engine::RuleEvaluator over the engine's default rules.
///
/// Pure C++ (no Qt types) so the same seed yields the same stream on every
/// platform and the logic is testable without an event loop. Random numbers
/// come from DemoRandom below, not <random> distributions, whose output the
/// standard leaves implementation-defined.
class DemoSimulator {
  public:
    static constexpr std::size_t kHistoryLimit = 500; // per device/sensor

    DemoSimulator(std::uint64_t seed, std::int64_t start_epoch_seconds, double anomaly_rate = 0.02);

    const std::vector<DeviceSpec>& devices() const { return devices_; }

    /// Advances one publish interval (1 s of simulated time).
    TickResult tick();
    /// Runs `ticks` intervals without publishing, so history is not empty on
    /// first load.
    void warm_up(int ticks);

    /// Samples the latest reading per sensor, in device order.
    std::vector<Reading> latest_readings() const;
    /// Stored readings for one sensor, oldest first, with timestamp >= since
    /// (string comparison; an empty `since` returns everything).
    std::vector<Reading> history(const std::string& device_id, const std::string& sensor,
                                 const std::string& since) const;

    /// Evaluates one reading against the current rules, the way the engine
    /// does, and records any alarms it raises.
    std::vector<alarm_engine::Alarm> ingest(const std::string& device_id, const std::string& sensor,
                                            double value, const std::string& timestamp);

    const std::vector<alarm_engine::Alarm>& alarms() const { return alarms_; }
    bool acknowledge(std::int64_t alarm_id);

    const std::vector<alarm_engine::Rule>& rules() const { return rules_; }
    std::int64_t add_rule(alarm_engine::Rule rule);
    bool update_rule(std::int64_t rule_id, alarm_engine::Rule rule);
    bool delete_rule(std::int64_t rule_id);

    std::string current_timestamp() const;

  private:
    using Key = std::pair<std::string, std::string>;

    class DemoRandom {
      public:
        explicit DemoRandom(std::uint64_t seed) : state_(seed) {}
        std::uint64_t next_u64();
        double uniform(); // [0, 1)
        double gauss(double mean, double stddev);

      private:
        std::uint64_t state_;
        bool has_spare_ = false;
        double spare_ = 0.0;
    } rng_;

    double anomaly_rate_;
    std::int64_t start_epoch_;
    int tick_ = 0;

    std::vector<DeviceSpec> devices_;
    std::vector<alarm_engine::Rule> rules_;
    std::vector<alarm_engine::Alarm> alarms_;
    std::map<Key, Reading> latest_;
    std::map<Key, std::vector<Reading>> history_;
    std::int64_t next_alarm_id_ = 1;
    std::int64_t next_rule_id_ = 1;
    alarm_engine::RuleEvaluator evaluator_;
};

} // namespace ui::demo
