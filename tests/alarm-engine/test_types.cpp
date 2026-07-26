#include <gtest/gtest.h>

#include "types.h"

using namespace alarm_engine;

TEST(TypeConversionTest, SeverityToString) {
    EXPECT_STREQ(severity_to_string(Severity::Info), "info");
    EXPECT_STREQ(severity_to_string(Severity::Warning), "warning");
    EXPECT_STREQ(severity_to_string(Severity::Critical), "critical");
}

TEST(TypeConversionTest, ConditionToString) {
    EXPECT_STREQ(condition_to_string(Condition::GreaterThan), "gt");
    EXPECT_STREQ(condition_to_string(Condition::LessThan), "lt");
    EXPECT_STREQ(condition_to_string(Condition::GreaterOrEqual), "gte");
    EXPECT_STREQ(condition_to_string(Condition::LessOrEqual), "lte");
    EXPECT_STREQ(condition_to_string(Condition::Equal), "eq");
}

TEST(TypeConversionTest, SeverityFromString) {
    EXPECT_EQ(severity_from_string("info"), Severity::Info);
    EXPECT_EQ(severity_from_string("warning"), Severity::Warning);
    EXPECT_EQ(severity_from_string("critical"), Severity::Critical);
}

TEST(TypeConversionTest, ConditionFromString) {
    EXPECT_EQ(condition_from_string("gt"), Condition::GreaterThan);
    EXPECT_EQ(condition_from_string("lt"), Condition::LessThan);
    EXPECT_EQ(condition_from_string("gte"), Condition::GreaterOrEqual);
    EXPECT_EQ(condition_from_string("lte"), Condition::LessOrEqual);
    EXPECT_EQ(condition_from_string("eq"), Condition::Equal);
}

TEST(TypeConversionTest, UnknownSeverityDefaultsToWarning) {
    EXPECT_EQ(severity_from_string("bogus"), Severity::Warning);
}

TEST(TypeConversionTest, UnknownConditionDefaultsToGreaterThan) {
    EXPECT_EQ(condition_from_string("bogus"), Condition::GreaterThan);
}

TEST(RuleTest, MatchesCorrectDevice) {
    Rule r;
    r.device_id = "pump-001";
    r.sensor = "temperature";
    EXPECT_TRUE(r.matches("pump-001", "temperature"));
}

TEST(RuleTest, DoesNotMatchWrongDevice) {
    Rule r;
    r.device_id = "pump-001";
    r.sensor = "temperature";
    EXPECT_FALSE(r.matches("pump-002", "temperature"));
}

TEST(RuleTest, DoesNotMatchWrongSensor) {
    Rule r;
    r.device_id = "pump-001";
    r.sensor = "temperature";
    EXPECT_FALSE(r.matches("pump-001", "pressure"));
}
