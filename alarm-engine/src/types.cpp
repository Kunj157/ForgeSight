#include "alarm-engine/types.h"

namespace alarm_engine {

const char* severity_to_string(Severity s) noexcept {
    switch (s) {
    case Severity::Info:
        return "info";
    case Severity::Warning:
        return "warning";
    case Severity::Critical:
        return "critical";
    }
    return "warning";
}

const char* condition_to_string(Condition c) noexcept {
    switch (c) {
    case Condition::GreaterThan:
        return "gt";
    case Condition::LessThan:
        return "lt";
    case Condition::GreaterOrEqual:
        return "gte";
    case Condition::LessOrEqual:
        return "lte";
    case Condition::Equal:
        return "eq";
    }
    return "gt";
}

Severity severity_from_string(const std::string& s) {
    if (s == "info")
        return Severity::Info;
    if (s == "critical")
        return Severity::Critical;
    return Severity::Warning;
}

Condition condition_from_string(const std::string& s) {
    if (s == "lt")
        return Condition::LessThan;
    if (s == "gte")
        return Condition::GreaterOrEqual;
    if (s == "lte")
        return Condition::LessOrEqual;
    if (s == "eq")
        return Condition::Equal;
    return Condition::GreaterThan;
}

} // namespace alarm_engine
