#include <gtest/gtest.h>

#include "ingestion/reading_parser.h"

using namespace ingestion;

class ReadingParserTest : public ::testing::Test {
protected:
    ReadingParser parser;
};

TEST_F(ReadingParserTest, ValidPayload) {
    auto result = parser.parse(R"({
        "device_id": "pump-001",
        "sensor": "temperature",
        "value": 65.3,
        "unit": "°C",
        "timestamp": "2026-07-26T10:00:00Z",
        "anomaly": false
    })");
    ASSERT_TRUE(std::holds_alternative<Reading>(result));
    auto& r = std::get<Reading>(result);
    EXPECT_EQ(r.device_id, "pump-001");
    EXPECT_EQ(r.sensor, "temperature");
    EXPECT_DOUBLE_EQ(r.value, 65.3);
    EXPECT_EQ(r.unit, "°C");
    EXPECT_FALSE(r.anomaly);
}

TEST_F(ReadingParserTest, MissingDeviceId) {
    auto result = parser.parse(R"({
        "sensor": "temperature",
        "value": 65.3,
        "unit": "°C",
        "timestamp": "2026-07-26T10:00:00Z"
    })");
    ASSERT_TRUE(std::holds_alternative<ParseError>(result));
    EXPECT_FALSE(std::get<ParseError>(result).reason.empty());
}

TEST_F(ReadingParserTest, MissingSensor) {
    auto result = parser.parse(R"({
        "device_id": "pump-001",
        "value": 65.3,
        "unit": "°C",
        "timestamp": "2026-07-26T10:00:00Z"
    })");
    ASSERT_TRUE(std::holds_alternative<ParseError>(result));
}

TEST_F(ReadingParserTest, MissingValue) {
    auto result = parser.parse(R"({
        "device_id": "pump-001",
        "sensor": "temperature",
        "unit": "°C",
        "timestamp": "2026-07-26T10:00:00Z"
    })");
    ASSERT_TRUE(std::holds_alternative<ParseError>(result));
}

TEST_F(ReadingParserTest, MissingTimestamp) {
    auto result = parser.parse(R"({
        "device_id": "pump-001",
        "sensor": "temperature",
        "value": 65.3,
        "unit": "°C"
    })");
    ASSERT_TRUE(std::holds_alternative<ParseError>(result));
}

TEST_F(ReadingParserTest, MalformedJson) {
    auto result = parser.parse("not json at all {{{");
    ASSERT_TRUE(std::holds_alternative<ParseError>(result));
}

TEST_F(ReadingParserTest, EmptyString) {
    auto result = parser.parse("");
    ASSERT_TRUE(std::holds_alternative<ParseError>(result));
}

TEST_F(ReadingParserTest, ValueIsNotNumber) {
    auto result = parser.parse(R"({
        "device_id": "pump-001",
        "sensor": "temperature",
        "value": "hot",
        "unit": "°C",
        "timestamp": "2026-07-26T10:00:00Z"
    })");
    ASSERT_TRUE(std::holds_alternative<ParseError>(result));
}

TEST_F(ReadingParserTest, NullPayload) {
    auto result = parser.parse("null");
    ASSERT_TRUE(std::holds_alternative<ParseError>(result));
}

TEST_F(ReadingParserTest, ArrayPayload) {
    auto result = parser.parse("[]");
    ASSERT_TRUE(std::holds_alternative<ParseError>(result));
}

TEST_F(ReadingParserTest, AnomalyFieldDefaultsFalse) {
    auto result = parser.parse(R"({
        "device_id": "pump-001",
        "sensor": "temperature",
        "value": 65.3,
        "unit": "°C",
        "timestamp": "2026-07-26T10:00:00Z"
    })");
    ASSERT_TRUE(std::holds_alternative<Reading>(result));
    EXPECT_FALSE(std::get<Reading>(result).anomaly);
}

TEST_F(ReadingParserTest, AnomalyFieldTrue) {
    auto result = parser.parse(R"({
        "device_id": "pump-001",
        "sensor": "temperature",
        "value": 120.0,
        "unit": "°C",
        "timestamp": "2026-07-26T10:00:00Z",
        "anomaly": true
    })");
    ASSERT_TRUE(std::holds_alternative<Reading>(result));
    EXPECT_TRUE(std::get<Reading>(result).anomaly);
}
