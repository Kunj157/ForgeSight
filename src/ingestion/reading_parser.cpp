#include "reading_parser.h"

#include <nlohmann/json.hpp>

namespace ingestion {

ParseResult ReadingParser::parse(const std::string& json_payload) const {
    if (json_payload.empty()) {
        return ParseError{"empty payload"};
    }

    nlohmann::json j;
    try {
        j = nlohmann::json::parse(json_payload);
    } catch (const nlohmann::json::parse_error& e) {
        return ParseError{std::string("invalid JSON: ") + e.what()};
    }

    if (!j.is_object()) {
        return ParseError{"payload is not a JSON object"};
    }

    Reading r;

    if (!j.contains("device_id") || !j["device_id"].is_string()) {
        return ParseError{"missing or invalid 'device_id'"};
    }
    r.device_id = j["device_id"].get<std::string>();

    if (!j.contains("sensor") || !j["sensor"].is_string()) {
        return ParseError{"missing or invalid 'sensor'"};
    }
    r.sensor = j["sensor"].get<std::string>();

    if (!j.contains("value") || !j["value"].is_number()) {
        return ParseError{"missing or invalid 'value'"};
    }
    r.value = j["value"].get<double>();

    if (!j.contains("unit") || !j["unit"].is_string()) {
        return ParseError{"missing or invalid 'unit'"};
    }
    r.unit = j["unit"].get<std::string>();

    if (!j.contains("timestamp") || !j["timestamp"].is_string()) {
        return ParseError{"missing or invalid 'timestamp'"};
    }
    r.timestamp = j["timestamp"].get<std::string>();

    if (j.contains("anomaly") && j["anomaly"].is_boolean()) {
        r.anomaly = j["anomaly"].get<bool>();
    }

    return r;
}

}  // namespace ingestion
