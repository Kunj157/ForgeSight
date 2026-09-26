#pragma once

#include <optional>
#include <string>
#include <vector>

#include "types.h"

namespace alarm_engine {

struct AlarmStoreConfig {
    std::string connection_string;
};

class AlarmStore {
  public:
    explicit AlarmStore(const AlarmStoreConfig& config);
    ~AlarmStore();

    AlarmStore(const AlarmStore&) = delete;
    AlarmStore& operator=(const AlarmStore&) = delete;

    bool is_connected() const;

    bool create_tables();

    bool add_rule(const Rule& rule);
    bool update_rule(const Rule& rule);
    bool delete_rule(std::int64_t rule_id);
    /// Current rules, or nullopt when the query failed or the store is disconnected.
    /// An empty vector means the table was read successfully and contains no rules.
    std::optional<std::vector<Rule>> try_load_rules() const;
    std::vector<Rule> load_rules() const;

    bool write_alarm(const Alarm& alarm);
    std::vector<Alarm> load_alarms(bool unacknowledged_only = false) const;
    bool acknowledge_alarm(std::int64_t alarm_id);

  private:
    void* conn_ = nullptr;
};

} // namespace alarm_engine
