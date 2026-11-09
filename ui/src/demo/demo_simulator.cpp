#include "ui/demo/demo_simulator.h"

#include "alarm-engine/default_rules.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>

namespace ui::demo {

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr std::size_t kAlarmLimit = 500;

// Mirrors simulators/config.yaml.
std::vector<DeviceSpec> config_devices() {
    return {
        {"pump-001",
         "Main Coolant Pump",
         "Plant A",
         "Floor 1",
         {
             {"temperature", "°C", 20.0, 95.0, 65.0, 3.0},
             {"vibration", "mm/s", 0.0, 20.0, 4.5, 1.0},
             {"pressure", "bar", 0.5, 5.0, 2.8, 0.2},
         }},
        {"compressor-001",
         "Air Compressor",
         "Plant A",
         "Floor 2",
         {
             {"temperature", "°C", 15.0, 110.0, 72.0, 5.0},
             {"current", "A", 0.0, 30.0, 12.0, 1.5},
         }},
    };
}

// Formats epoch seconds as ISO-8601 UTC without touching the C library's
// time zone or locale state (civil-from-days, H. Hinnant).
std::string format_utc(std::int64_t t) {
    std::int64_t days = t >= 0 ? t / 86400 : (t - 86399) / 86400;
    std::int64_t secs = t - days * 86400;
    days += 719468;
    const std::int64_t era = (days >= 0 ? days : days - 146096) / 146097;
    const std::int64_t doe = days - era * 146097;
    const std::int64_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const std::int64_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const std::int64_t mp = (5 * doy + 2) / 153;
    const std::int64_t day = doy - (153 * mp + 2) / 5 + 1;
    const std::int64_t month = mp < 10 ? mp + 3 : mp - 9;
    const std::int64_t year = yoe + era * 400 + (month <= 2 ? 1 : 0);

    char buf[96];
    std::snprintf(buf, sizeof buf, "%04lld-%02lld-%02lldT%02lld:%02lld:%02lldZ",
                  static_cast<long long>(year), static_cast<long long>(month),
                  static_cast<long long>(day), static_cast<long long>(secs / 3600),
                  static_cast<long long>(secs % 3600 / 60), static_cast<long long>(secs % 60));
    return buf;
}

} // namespace

// splitmix64 for integers; uniform() uses the top 53 bits, gauss() is
// Box-Muller with a cached spare. Only the integer step is bit-exact across
// platforms; log/sin/cos may differ in the last ulp between libms.
std::uint64_t DemoSimulator::DemoRandom::next_u64() {
    state_ += 0x9E3779B97F4A7C15ULL;
    std::uint64_t z = state_;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

double DemoSimulator::DemoRandom::uniform() {
    return static_cast<double>(next_u64() >> 11) * (1.0 / 9007199254740992.0);
}

double DemoSimulator::DemoRandom::gauss(double mean, double stddev) {
    if (has_spare_) {
        has_spare_ = false;
        return mean + stddev * spare_;
    }
    double u1 = uniform();
    while (u1 <= 0.0)
        u1 = uniform();
    const double u2 = uniform();
    const double radius = std::sqrt(-2.0 * std::log(u1));
    const double theta = 2.0 * kPi * u2;
    spare_ = radius * std::sin(theta);
    has_spare_ = true;
    return mean + stddev * radius * std::cos(theta);
}

DemoSimulator::DemoSimulator(std::uint64_t seed, std::int64_t start_epoch_seconds,
                             double anomaly_rate)
    : rng_(seed), anomaly_rate_(anomaly_rate), start_epoch_(start_epoch_seconds),
      devices_(config_devices()) {
    for (auto rule : alarm_engine::default_rules()) {
        rule.id = next_rule_id_++;
        rules_.push_back(std::move(rule));
    }
}

std::string DemoSimulator::current_timestamp() const {
    return format_utc(start_epoch_ + tick_);
}

TickResult DemoSimulator::tick() {
    TickResult out;
    const std::string ts = current_timestamp();

    for (const auto& device : devices_) {
        for (const auto& sensor : device.sensors) {
            double value = rng_.gauss(sensor.normal_mean, sensor.normal_std);
            bool anomaly = false;
            if (rng_.uniform() < anomaly_rate_) {
                anomaly = true;
                const double spike =
                    (0.3 + 0.5 * rng_.uniform()) * (sensor.max - sensor.normal_mean);
                value = sensor.normal_mean + spike;
            }
            value = std::clamp(std::round(value * 100.0) / 100.0, sensor.min, sensor.max);

            Reading reading{device.id, sensor.name, sensor.unit, ts, value, anomaly};
            const Key key{device.id, sensor.name};
            latest_[key] = reading;

            auto& past = history_[key];
            past.push_back(reading);
            if (past.size() > kHistoryLimit)
                past.erase(past.begin(),
                           past.begin() + static_cast<std::ptrdiff_t>(past.size() - kHistoryLimit));

            auto fired = ingest(device.id, sensor.name, value, ts);
            out.new_alarms.insert(out.new_alarms.end(), fired.begin(), fired.end());
            out.readings.push_back(std::move(reading));
        }
    }

    ++tick_;
    return out;
}

void DemoSimulator::warm_up(int ticks) {
    for (int i = 0; i < ticks; ++i)
        tick();
}

std::vector<Reading> DemoSimulator::latest_readings() const {
    std::vector<Reading> out;
    for (const auto& device : devices_) {
        for (const auto& sensor : device.sensors) {
            auto it = latest_.find({device.id, sensor.name});
            if (it != latest_.end())
                out.push_back(it->second);
        }
    }
    return out;
}

std::vector<Reading> DemoSimulator::history(const std::string& device_id, const std::string& sensor,
                                            const std::string& since) const {
    std::vector<Reading> out;
    auto it = history_.find({device_id, sensor});
    if (it == history_.end())
        return out;
    for (const auto& reading : it->second) {
        if (since.empty() || reading.timestamp >= since)
            out.push_back(reading);
    }
    return out;
}

std::vector<alarm_engine::Alarm> DemoSimulator::ingest(const std::string& device_id,
                                                       const std::string& sensor, double value,
                                                       const std::string& timestamp) {
    auto fired = evaluator_.evaluate_all(rules_, device_id, sensor, value, timestamp);
    for (auto& alarm : fired) {
        alarm.id = next_alarm_id_++;
        alarms_.push_back(alarm);
    }
    if (alarms_.size() > kAlarmLimit)
        alarms_.erase(alarms_.begin(),
                      alarms_.begin() + static_cast<std::ptrdiff_t>(alarms_.size() - kAlarmLimit));
    return fired;
}

bool DemoSimulator::acknowledge(std::int64_t alarm_id) {
    for (auto& alarm : alarms_) {
        if (alarm.id == alarm_id) {
            alarm.acknowledged = true;
            return true;
        }
    }
    return false;
}

std::int64_t DemoSimulator::add_rule(alarm_engine::Rule rule) {
    rule.id = next_rule_id_++;
    rules_.push_back(rule);
    return rule.id;
}

bool DemoSimulator::update_rule(std::int64_t rule_id, alarm_engine::Rule rule) {
    for (auto& existing : rules_) {
        if (existing.id == rule_id) {
            rule.id = rule_id;
            existing = std::move(rule);
            return true;
        }
    }
    return false;
}

bool DemoSimulator::delete_rule(std::int64_t rule_id) {
    auto it = std::find_if(rules_.begin(), rules_.end(),
                           [rule_id](const alarm_engine::Rule& r) { return r.id == rule_id; });
    if (it == rules_.end())
        return false;
    rules_.erase(it);
    return true;
}

} // namespace ui::demo
