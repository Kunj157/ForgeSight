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
    std::string plant = "Unassigned";
    std::string floor = "Unassigned";
};

class DeviceService {
  public:
    explicit DeviceService(void* conn);

    std::vector<DeviceInfo> list_devices() const;
    std::vector<ingestion::Reading> list_latest_readings() const;
    std::vector<ingestion::Reading> get_history(const std::string& device_id,
                                                const std::string& sensor,
                                                const std::string& since) const;

    std::vector<ingestion::Reading> get_readings_since(const std::string& since) const;

    /// Assigns (or reassigns) the plant/floor location metadata for a device.
    /// Upserted independently of readings, so it survives even if the device
    /// hasn't reported in yet.
    bool set_device_location(const std::string& device_id, const std::string& plant,
                             const std::string& floor) const;

    /// Like set_device_location(), but leaves any existing assignment alone
    /// (ON CONFLICT DO NOTHING). Used at startup to give known simulator
    /// devices a sensible default location without clobbering a location that
    /// was already deliberately set.
    bool seed_default_location(const std::string& device_id, const std::string& plant,
                               const std::string& floor) const;

  private:
    void ensure_metadata_table() const;

    void* conn_;
};

} // namespace api
