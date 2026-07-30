#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "ingestion/reading.h"

namespace api {

struct DeviceInfo {
    std::string id;
    std::string name;
    std::string sensor;
    std::string last_reading_time;
    double last_value = 0.0;
    std::string last_unit;
    bool anomaly = false;
};

class DeviceService {
public:
    explicit DeviceService(void* conn);

    std::vector<DeviceInfo> list_devices() const;
    std::vector<ingestion::Reading> list_latest_readings() const;
    std::vector<ingestion::Reading> get_history(
        const std::string& device_id,
        const std::string& sensor,
        const std::string& since) const;

    std::vector<ingestion::Reading> get_readings_since(
        const std::string& since) const;

private:
    void* conn_;
};

}  // namespace api
