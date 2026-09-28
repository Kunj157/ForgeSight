#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "ingestion/reading.h"

namespace api {

struct PolledReading {
    std::int64_t id = 0;
    ingestion::Reading reading;
};

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
    /// Readings for a device+sensor since `since`, in chronological order.
    ///
    /// When `max_points > 0` and the window holds more than that many rows, the
    /// result is downsampled to roughly `max_points` evenly-spaced rows (plus
    /// any anomaly rows and the final row, which are always kept). This bounds
    /// the payload/render cost for wide time ranges — a 24h window at 1 Hz is
    /// ~86k rows, which would otherwise freeze the chart. The default (1000) is
    /// about a chart's worth of horizontal pixels, so more points add no visual
    /// value. `max_points <= 0` disables downsampling and returns every row.
    std::vector<ingestion::Reading> get_history(const std::string& device_id,
                                                const std::string& sensor, const std::string& since,
                                                int max_points = 1000) const;

    /// Live-poll cursor. Id of the row just before `catchup_since`, or the
    /// current max id when nothing is in that window. Steady-state polling
    /// uses `readings_after` so a later row that shares a timestamp is still
    /// delivered once, and a row already delivered is not sent again.
    /// Timestamp cursors cannot do both: `>=` replays the whole timestamp
    /// forever, and `>` drops a commit that lands on the cursor time.
    /// Empty when the lookup fails (table missing, connection down). Callers
    /// must retry rather than treat that as id 0, which would replay history.
    std::optional<std::int64_t> reading_poll_cursor(const std::string& catchup_since) const;
    std::vector<PolledReading> readings_after(std::int64_t after_id) const;

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
