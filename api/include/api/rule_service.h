#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "alarm-engine/types.h"

namespace api {

class RuleService {
  public:
    explicit RuleService(void* conn);

    std::vector<alarm_engine::Rule> list_rules() const;
    std::optional<alarm_engine::Rule> get_rule(std::int64_t id) const;
    std::int64_t create_rule(const alarm_engine::Rule& rule);
    bool update_rule(const alarm_engine::Rule& rule);
    bool delete_rule(std::int64_t id);

    /// Same id cursor as DeviceService::reading_poll_cursor, for alarms.
    /// A timestamp cursor drops a later alarm that shares the cursor time
    /// (one simulator sweep stamps every sensor together, and the two halves
    /// of that sweep can commit separately).
    /// Empty when the lookup fails. Callers retry; id 0 would replay every alarm.
    std::optional<std::int64_t> alarm_poll_cursor(const std::string& catchup_since) const;
    std::vector<alarm_engine::Alarm> alarms_after(std::int64_t after_id) const;
    std::vector<alarm_engine::Alarm> list_alarms(bool unacknowledged_only = false) const;
    bool acknowledge_alarm(std::int64_t id);

  private:
    void* conn_;
};

} // namespace api
