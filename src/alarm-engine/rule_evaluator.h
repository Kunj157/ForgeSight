#pragma once

#include <string>
#include <vector>

#include "types.h"

namespace alarm_engine {

class RuleEvaluator {
public:
    bool evaluate(const Rule& rule, double value) const;
    std::string format_message(const Rule& rule, double value) const;
    std::vector<Alarm> evaluate_all(const std::vector<Rule>& rules,
                                    const std::string& device_id,
                                    const std::string& sensor,
                                    double value,
                                    const std::string& timestamp) const;
};

}  // namespace alarm_engine
