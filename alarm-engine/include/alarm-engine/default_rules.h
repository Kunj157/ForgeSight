#pragma once

#include <vector>

#include "types.h"

namespace alarm_engine {

/// The rule set `alarm-engine --seed` installs into an empty rules table.
/// Ids are left at 0; the store (or an in-memory simulator) assigns them.
std::vector<Rule> default_rules();

} // namespace alarm_engine
