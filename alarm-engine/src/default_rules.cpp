#include "alarm-engine/default_rules.h"

namespace alarm_engine {

std::vector<Rule> default_rules() {
    return {
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
}

} // namespace alarm_engine
