#pragma once

#include <optional>
#include <utility>
#include <vector>

#include "types.h"

namespace alarm_engine {

/// Rule set to evaluate on this poll.
///
/// A successful load replaces the previous set, including when it is empty —
/// that is how a deleted rule stops firing. A failed load keeps the previous
/// set so a transient query error does not disable alarming for the readings
/// about to be evaluated (those readings are not revisited).
inline std::vector<Rule> rules_for_poll(std::optional<std::vector<Rule>> loaded,
                                        std::vector<Rule> previous) {
    if (loaded.has_value())
        return std::move(*loaded);
    return previous;
}

} // namespace alarm_engine
