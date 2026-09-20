#include "alarm-engine/rule_evaluator.h"

#include <sstream>

namespace alarm_engine {

bool RuleEvaluator::evaluate(const Rule& rule, double value) const {
    switch (rule.condition) {
    case Condition::GreaterThan:
        return value > rule.threshold;
    case Condition::LessThan:
        return value < rule.threshold;
    case Condition::GreaterOrEqual:
        return value >= rule.threshold;
    case Condition::LessOrEqual:
        return value <= rule.threshold;
    case Condition::Equal:
        return value == rule.threshold;
    }
    return false;
}

namespace {

// Describes how a value relates to its threshold, per condition, so an
// auto-generated message reads naturally ("exceeded" / "below" / "reached").
std::string condition_phrase(Condition condition) {
    switch (condition) {
    case Condition::GreaterThan:
    case Condition::GreaterOrEqual:
        return "exceeded threshold";
    case Condition::LessThan:
    case Condition::LessOrEqual:
        return "below threshold";
    case Condition::Equal:
        return "reached threshold";
    }
    return "crossed threshold";
}

} // namespace

std::string RuleEvaluator::default_message_template(const Rule& rule) const {
    return "{device} {sensor} value {value} " + condition_phrase(rule.condition) + " {threshold}";
}

std::string RuleEvaluator::format_message(const Rule& rule, double value) const {
    // Fall back to a descriptive auto-generated template when a rule has no
    // message configured, so an alarm is never rendered with a blank message.
    std::string msg =
        rule.message_template.empty() ? default_message_template(rule) : rule.message_template;

    auto replace = [&](const std::string& placeholder, const std::string& replacement) {
        std::size_t pos = 0;
        while ((pos = msg.find(placeholder, pos)) != std::string::npos) {
            msg.replace(pos, placeholder.size(), replacement);
            pos += replacement.size();
        }
    };

    replace("{device}", rule.device_id);
    replace("{sensor}", rule.sensor);

    std::ostringstream val_ss;
    val_ss << value;
    replace("{value}", val_ss.str());

    std::ostringstream thr_ss;
    thr_ss << rule.threshold;
    replace("{threshold}", thr_ss.str());

    replace("{severity}", severity_to_string(rule.severity));

    return msg;
}

std::vector<Alarm> RuleEvaluator::evaluate_all(const std::vector<Rule>& rules,
                                               const std::string& device_id,
                                               const std::string& sensor, double value,
                                               const std::string& timestamp) const {

    std::vector<Alarm> alarms;

    for (const auto& rule : rules) {
        if (!rule.matches(device_id, sensor))
            continue;
        if (!evaluate(rule, value))
            continue;

        Alarm alarm;
        alarm.rule_id = rule.id;
        alarm.device_id = device_id;
        alarm.sensor = sensor;
        alarm.value = value;
        alarm.severity = rule.severity;
        alarm.message = format_message(rule, value);
        alarm.timestamp = timestamp;
        alarm.acknowledged = false;

        alarms.push_back(std::move(alarm));
    }

    return alarms;
}

} // namespace alarm_engine
