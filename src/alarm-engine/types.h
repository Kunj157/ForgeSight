#pragma once

#include <cstdint>
#include <string>

namespace alarm_engine {

enum class Condition : std::uint8_t {
    GreaterThan,
    LessThan,
    GreaterOrEqual,
    LessOrEqual,
    Equal,
};

enum class Severity : std::uint8_t {
    Info,
    Warning,
    Critical,
};

struct Rule {
    std::int64_t id = 0;
    std::string device_id;
    std::string sensor;
    Condition condition = Condition::GreaterThan;
    double threshold = 0.0;
    Severity severity = Severity::Warning;
    std::string message_template;

    bool matches(const std::string& dev, const std::string& sen) const {
        return device_id == dev && sensor == sen;
    }
};

struct Alarm {
    std::int64_t id = 0;
    std::int64_t rule_id = 0;
    std::string device_id;
    std::string sensor;
    double value = 0.0;
    Severity severity = Severity::Warning;
    std::string message;
    std::string timestamp;
    bool acknowledged = false;
};

const char* severity_to_string(Severity s) noexcept;
const char* condition_to_string(Condition c) noexcept;
Severity severity_from_string(const std::string& s);
Condition condition_from_string(const std::string& s);

}  // namespace alarm_engine
