#pragma once

#include <string>
#include <vector>

#include "types.h"

namespace api {

class RuleService {
public:
    explicit RuleService(void* conn);

    std::vector<alarm_engine::Rule> list_rules() const;
    std::optional<alarm_engine::Rule> get_rule(std::int64_t id) const;
    std::int64_t create_rule(const alarm_engine::Rule& rule);
    bool update_rule(const alarm_engine::Rule& rule);
    bool delete_rule(std::int64_t id);

private:
    void* conn_;
};

}  // namespace api
