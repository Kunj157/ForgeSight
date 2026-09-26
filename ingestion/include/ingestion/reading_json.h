#pragma once

#include <string>

#include "reading.h"

namespace ingestion {

/// Serialize a reading to the same JSON object the MQTT parser accepts
/// (`device_id`, `sensor`, `value`, `unit`, `timestamp`, `anomaly`). Used as
/// the Kafka payload so a downstream consumer can reuse ReadingParser.
std::string reading_to_json(const Reading& r);

} // namespace ingestion
