#include <gtest/gtest.h>

#include "ui/demo/demo_simulator.h"

#include <cstdint>
#include <string>
#include <vector>

using ui::demo::DemoSimulator;
using ui::demo::Reading;
using ui::demo::SensorSpec;

namespace {

constexpr std::int64_t kStart = 1767225600; // 2026-01-01T00:00:00Z
const char* kTs0 = "2026-01-01T00:00:00Z";

bool same_reading(const Reading& a, const Reading& b) {
    return a.device_id == b.device_id && a.sensor == b.sensor && a.unit == b.unit &&
           a.timestamp == b.timestamp && a.value == b.value && a.anomaly == b.anomaly;
}

const SensorSpec* find_sensor(const DemoSimulator& sim, const std::string& device,
                              const std::string& sensor) {
    for (const auto& d : sim.devices()) {
        if (d.id != device)
            continue;
        for (const auto& s : d.sensors) {
            if (s.name == sensor)
                return &s;
        }
    }
    return nullptr;
}

} // namespace

TEST(DemoSimulator, SameSeedProducesSameStream) {
    DemoSimulator a(7, kStart);
    DemoSimulator b(7, kStart);
    for (int t = 0; t < 60; ++t) {
        const auto ra = a.tick().readings;
        const auto rb = b.tick().readings;
        ASSERT_EQ(ra.size(), rb.size());
        for (std::size_t i = 0; i < ra.size(); ++i)
            EXPECT_TRUE(same_reading(ra[i], rb[i])) << "tick " << t << " reading " << i;
    }
}

TEST(DemoSimulator, DifferentSeedsDiverge) {
    DemoSimulator a(7, kStart);
    DemoSimulator b(8, kStart);
    const auto ra = a.tick().readings;
    const auto rb = b.tick().readings;
    ASSERT_EQ(ra.size(), rb.size());
    bool differs = false;
    for (std::size_t i = 0; i < ra.size(); ++i)
        differs = differs || ra[i].value != rb[i].value;
    EXPECT_TRUE(differs);
}

TEST(DemoSimulator, EmitsOneReadingPerSensorInConfigOrder) {
    DemoSimulator sim(1, kStart);
    const auto readings = sim.tick().readings;
    ASSERT_EQ(readings.size(), 5u);
    EXPECT_EQ(readings[0].device_id, "pump-001");
    EXPECT_EQ(readings[0].sensor, "temperature");
    EXPECT_EQ(readings[0].unit, "°C");
    EXPECT_EQ(readings[4].device_id, "compressor-001");
    EXPECT_EQ(readings[4].sensor, "current");
}

TEST(DemoSimulator, TimestampsAreUtcIsoAndAdvancePerTick) {
    DemoSimulator sim(1, kStart);
    EXPECT_EQ(sim.tick().readings.front().timestamp, kTs0);
    EXPECT_EQ(sim.tick().readings.front().timestamp, "2026-01-01T00:00:01Z");
}

TEST(DemoSimulator, TimestampRollsOverMidnight) {
    DemoSimulator sim(1, kStart + 86399);
    EXPECT_EQ(sim.tick().readings.front().timestamp, "2026-01-01T23:59:59Z");
    EXPECT_EQ(sim.tick().readings.front().timestamp, "2026-01-02T00:00:00Z");
}

TEST(DemoSimulator, ValuesStayWithinSensorRangeAndAnomaliesAreFlagged) {
    DemoSimulator sim(3, kStart, 1.0);
    for (int t = 0; t < 50; ++t) {
        const auto readings = sim.tick().readings;
        for (const auto& r : readings) {
            const SensorSpec* spec = find_sensor(sim, r.device_id, r.sensor);
            ASSERT_NE(spec, nullptr);
            EXPECT_GE(r.value, spec->min);
            EXPECT_LE(r.value, spec->max);
            EXPECT_TRUE(r.anomaly); // anomaly_rate 1.0
        }
    }
}

TEST(DemoSimulator, NoAnomaliesWhenRateIsZero) {
    DemoSimulator sim(3, kStart, 0.0);
    for (int t = 0; t < 50; ++t) {
        for (const auto& r : sim.tick().readings)
            EXPECT_FALSE(r.anomaly);
    }
}

TEST(DemoSimulator, DefaultPressureRuleFiresOnExpectedReading) {
    DemoSimulator sim(1, kStart, 0.0);
    const auto rules = sim.rules();
    ASSERT_GE(rules.size(), 3u);
    // Third entry of alarm_engine::default_rules(): pump-001 pressure < 1.0, critical.
    const auto fired = sim.ingest("pump-001", "pressure", 0.8, kTs0);
    ASSERT_EQ(fired.size(), 1u);
    EXPECT_EQ(fired[0].rule_id, rules[2].id);
    EXPECT_EQ(fired[0].severity, alarm_engine::Severity::Critical);
    EXPECT_EQ(fired[0].message, "pump-001 pressure critically low (0.8 < 1)");
    EXPECT_EQ(sim.alarms().size(), 1u);
}

TEST(DemoSimulator, OverlappingTemperatureRulesBothFire) {
    DemoSimulator sim(1, kStart, 0.0);
    // 96 C exceeds both the pump warning (80) and critical (95) defaults.
    EXPECT_EQ(sim.ingest("pump-001", "temperature", 96.0, kTs0).size(), 2u);
}

TEST(DemoSimulator, AcknowledgeMarksAlarmAndRejectsUnknownIds) {
    DemoSimulator sim(1, kStart, 0.0);
    const auto fired = sim.ingest("pump-001", "pressure", 0.8, kTs0);
    ASSERT_EQ(fired.size(), 1u);
    const auto id = fired[0].id;
    EXPECT_GT(id, 0);
    EXPECT_FALSE(sim.alarms().front().acknowledged);

    EXPECT_TRUE(sim.acknowledge(id));
    EXPECT_TRUE(sim.alarms().front().acknowledged);
    EXPECT_FALSE(sim.acknowledge(9999));
}

TEST(DemoSimulator, RuleCreateUpdateDeleteChangesEvaluation) {
    DemoSimulator sim(1, kStart, 0.0);
    alarm_engine::Rule rule;
    rule.device_id = "compressor-001";
    rule.sensor = "current";
    rule.condition = alarm_engine::Condition::GreaterThan;
    rule.threshold = 5.0;
    rule.severity = alarm_engine::Severity::Info;

    const auto id = sim.add_rule(rule);
    EXPECT_GT(id, 8); // after the eight defaults
    EXPECT_EQ(sim.ingest("compressor-001", "current", 6.0, kTs0).size(), 1u);

    alarm_engine::Rule raised = rule;
    raised.threshold = 50.0;
    EXPECT_TRUE(sim.update_rule(id, raised));
    EXPECT_TRUE(sim.ingest("compressor-001", "current", 6.0, kTs0).empty());

    EXPECT_TRUE(sim.delete_rule(id));
    EXPECT_FALSE(sim.delete_rule(id));
    EXPECT_FALSE(sim.update_rule(id, raised));
}

TEST(DemoSimulator, HistoryKeepsTicksAndFiltersBySince) {
    DemoSimulator sim(1, kStart, 0.0);
    sim.warm_up(5);
    EXPECT_EQ(sim.history("pump-001", "temperature", "").size(), 5u);
    EXPECT_EQ(sim.history("pump-001", "temperature", "2026-01-01T00:00:03Z").size(), 2u);
    EXPECT_TRUE(sim.history("no-such-device", "temperature", "").empty());
}

TEST(DemoSimulator, LatestReadingsHoldOnePerSensor) {
    DemoSimulator sim(1, kStart, 0.0);
    sim.tick();
    sim.tick();
    const auto latest = sim.latest_readings();
    ASSERT_EQ(latest.size(), 5u);
    EXPECT_EQ(latest[0].timestamp, "2026-01-01T00:00:01Z");
}

TEST(DemoSimulator, AnomalousTicksRaiseDefaultAlarms) {
    DemoSimulator sim(5, kStart, 1.0);
    std::size_t raised = 0;
    for (int t = 0; t < 100; ++t)
        raised += sim.tick().new_alarms.size();
    EXPECT_GT(raised, 0u);
}
