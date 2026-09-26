#include "ingestion/reading_json.h"

#include <nlohmann/json.hpp>

namespace ingestion {

std::string reading_to_json(const Reading& r) {
    nlohmann::json j = {{"device_id", r.device_id}, {"sensor", r.sensor},
                        {"value", r.value},         {"unit", r.unit},
                        {"timestamp", r.timestamp}, {"anomaly", r.anomaly}};
    return j.dump();
}

} // namespace ingestion
