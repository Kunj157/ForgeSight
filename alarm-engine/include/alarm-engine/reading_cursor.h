#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "ingestion/reading.h"

namespace alarm_engine {

/// One readings row observed by the alarm poll.
struct PolledReading {
    std::int64_t id = 0;
    ingestion::Reading reading;
};

/// Readings committed after \p after_id, in insert order.
///
/// The poll must key off the serial id, not the device timestamp. A single
/// sample sweep shares one timestamp and can land in the table across two
/// flushes; a `timestamp > last` cursor permanently skips the second flush,
/// so those readings never raise alarms. Ids are assigned in commit order.
std::vector<PolledReading> fetch_readings_after(void* conn, std::int64_t after_id);

/// Highest reading id currently committed, or 0 when the table is empty.
/// nullopt when the cursor cannot be read — callers must not treat that as 0,
/// or a failed startup query would re-alarm every stored reading.
std::optional<std::int64_t> catch_up_reading_id(void* conn);

} // namespace alarm_engine
