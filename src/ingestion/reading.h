#pragma once

#include <chrono>
#include <optional>
#include <string>

namespace ingestion {

struct Reading {
    std::string device_id;
    std::string sensor;
    double value = 0.0;
    std::string unit;
    std::string timestamp;
    bool anomaly = false;

    bool operator==(const Reading& o) const noexcept {
        return device_id == o.device_id && sensor == o.sensor &&
               value == o.value && unit == o.unit &&
               timestamp == o.timestamp && anomaly == o.anomaly;
    }
};

struct ReadingEvent {
    Reading reading;
    std::chrono::system_clock::time_point received_at;
};

}  // namespace ingestion
