#include <gtest/gtest.h>

#include "alarm-engine/rule_evaluator.h"
#include "alarm-engine/types.h"

using namespace alarm_engine;

class RuleEvaluatorTest : public ::testing::Test {
  protected:
    RuleEvaluator eval;

    Rule make_rule(Condition cond, double threshold, Severity sev = Severity::Warning) {
        Rule r;
        r.id = 1;
        r.device_id = "pump-001";
        r.sensor = "temperature";
        r.condition = cond;
        r.threshold = threshold;
        r.severity = sev;
        r.message_template = "{device} {sensor} value {value} exceeded threshold {threshold}";
        return r;
    }
};

TEST_F(RuleEvaluatorTest, GreaterThanTriggers) {
    auto rule = make_rule(Condition::GreaterThan, 80.0);
    EXPECT_TRUE(eval.evaluate(rule, 85.0));
}

TEST_F(RuleEvaluatorTest, GreaterThanDoesNotTrigger) {
    auto rule = make_rule(Condition::GreaterThan, 80.0);
    EXPECT_FALSE(eval.evaluate(rule, 75.0));
}

TEST_F(RuleEvaluatorTest, GreaterThanBoundaryExact) {
    auto rule = make_rule(Condition::GreaterThan, 80.0);
    EXPECT_FALSE(eval.evaluate(rule, 80.0));
}

TEST_F(RuleEvaluatorTest, LessThanTriggers) {
    auto rule = make_rule(Condition::LessThan, 20.0);
    EXPECT_TRUE(eval.evaluate(rule, 15.0));
}

TEST_F(RuleEvaluatorTest, LessThanDoesNotTrigger) {
    auto rule = make_rule(Condition::LessThan, 20.0);
    EXPECT_FALSE(eval.evaluate(rule, 25.0));
}

TEST_F(RuleEvaluatorTest, LessThanBoundaryExact) {
    auto rule = make_rule(Condition::LessThan, 20.0);
    EXPECT_FALSE(eval.evaluate(rule, 20.0));
}

TEST_F(RuleEvaluatorTest, GreaterOrEqualTriggersAbove) {
    auto rule = make_rule(Condition::GreaterOrEqual, 80.0);
    EXPECT_TRUE(eval.evaluate(rule, 85.0));
}

TEST_F(RuleEvaluatorTest, GreaterOrEqualTriggersExact) {
    auto rule = make_rule(Condition::GreaterOrEqual, 80.0);
    EXPECT_TRUE(eval.evaluate(rule, 80.0));
}

TEST_F(RuleEvaluatorTest, GreaterOrEqualDoesNotTrigger) {
    auto rule = make_rule(Condition::GreaterOrEqual, 80.0);
    EXPECT_FALSE(eval.evaluate(rule, 79.0));
}

TEST_F(RuleEvaluatorTest, LessOrEqualTriggersBelow) {
    auto rule = make_rule(Condition::LessOrEqual, 20.0);
    EXPECT_TRUE(eval.evaluate(rule, 15.0));
}

TEST_F(RuleEvaluatorTest, LessOrEqualTriggersExact) {
    auto rule = make_rule(Condition::LessOrEqual, 20.0);
    EXPECT_TRUE(eval.evaluate(rule, 20.0));
}

TEST_F(RuleEvaluatorTest, LessOrEqualDoesNotTrigger) {
    auto rule = make_rule(Condition::LessOrEqual, 20.0);
    EXPECT_FALSE(eval.evaluate(rule, 21.0));
}

TEST_F(RuleEvaluatorTest, EqualTriggers) {
    auto rule = make_rule(Condition::Equal, 65.0);
    EXPECT_TRUE(eval.evaluate(rule, 65.0));
}

TEST_F(RuleEvaluatorTest, EqualDoesNotTrigger) {
    auto rule = make_rule(Condition::Equal, 65.0);
    EXPECT_FALSE(eval.evaluate(rule, 66.0));
}

TEST_F(RuleEvaluatorTest, MultipleRulesMultipleFires) {
    std::vector<Rule> rules = {
        make_rule(Condition::GreaterThan, 80.0, Severity::Warning),
        make_rule(Condition::GreaterThan, 90.0, Severity::Critical),
        make_rule(Condition::LessThan, 10.0, Severity::Warning),
    };

    auto alarms = eval.evaluate_all(rules, "pump-001", "temperature", 95.0, "2026-07-26T10:00:00Z");
    EXPECT_EQ(alarms.size(), 2u);
    EXPECT_EQ(alarms[0].severity, Severity::Warning);
    EXPECT_EQ(alarms[1].severity, Severity::Critical);
}

TEST_F(RuleEvaluatorTest, NoRulesFire) {
    std::vector<Rule> rules = {
        make_rule(Condition::GreaterThan, 80.0),
        make_rule(Condition::LessThan, 10.0),
    };

    auto alarms = eval.evaluate_all(rules, "pump-001", "temperature", 50.0, "2026-07-26T10:00:00Z");
    EXPECT_TRUE(alarms.empty());
}

TEST_F(RuleEvaluatorTest, NonMatchingRuleIgnored) {
    Rule rule = make_rule(Condition::GreaterThan, 80.0);
    rule.device_id = "other-device";

    std::vector<Rule> rules = {rule};
    auto alarms = eval.evaluate_all(rules, "pump-001", "temperature", 90.0, "2026-07-26T10:00:00Z");
    EXPECT_TRUE(alarms.empty());
}

TEST_F(RuleEvaluatorTest, FormatMessage) {
    auto rule = make_rule(Condition::GreaterThan, 80.0);
    auto msg = eval.format_message(rule, 85.0);
    EXPECT_EQ(msg, "pump-001 temperature value 85 exceeded threshold 80");
}

TEST_F(RuleEvaluatorTest, FormatMessageFallsBackWhenTemplateEmpty) {
    // A rule with no template (e.g. seeded before templates existed, or
    // created via the API without one) must still yield a descriptive
    // message rather than an empty string.
    auto rule = make_rule(Condition::GreaterThan, 80.0);
    rule.message_template = "";
    auto msg = eval.format_message(rule, 85.0);

    EXPECT_FALSE(msg.empty());
    EXPECT_NE(msg.find("pump-001"), std::string::npos);
    EXPECT_NE(msg.find("temperature"), std::string::npos);
    EXPECT_NE(msg.find("85"), std::string::npos);
    EXPECT_NE(msg.find("80"), std::string::npos);
}

TEST_F(RuleEvaluatorTest, FormatMessageFallbackReflectsCondition) {
    auto rule = make_rule(Condition::LessThan, 1.0, Severity::Critical);
    rule.sensor = "pressure";
    rule.message_template = "";
    auto msg = eval.format_message(rule, 0.4);

    EXPECT_FALSE(msg.empty());
    EXPECT_NE(msg.find("pressure"), std::string::npos);
    // "below" for a LessThan rule, not "exceeded".
    EXPECT_NE(msg.find("below"), std::string::npos);
}

TEST_F(RuleEvaluatorTest, EvaluateAllNeverProducesEmptyMessage) {
    auto rule = make_rule(Condition::GreaterThan, 80.0, Severity::Warning);
    rule.message_template = "";
    std::vector<Rule> rules = {rule};

    auto alarms = eval.evaluate_all(rules, "pump-001", "temperature", 90.0, "2026-07-26T10:00:00Z");
    ASSERT_EQ(alarms.size(), 1u);
    EXPECT_FALSE(alarms[0].message.empty());
}

TEST_F(RuleEvaluatorTest, AlarmContainsCorrectFields) {
    std::vector<Rule> rules = {
        make_rule(Condition::GreaterThan, 80.0, Severity::Critical),
    };

    auto alarms = eval.evaluate_all(rules, "pump-001", "temperature", 95.0, "2026-07-26T10:00:00Z");
    ASSERT_EQ(alarms.size(), 1u);
    EXPECT_EQ(alarms[0].device_id, "pump-001");
    EXPECT_EQ(alarms[0].sensor, "temperature");
    EXPECT_DOUBLE_EQ(alarms[0].value, 95.0);
    EXPECT_EQ(alarms[0].severity, Severity::Critical);
    EXPECT_EQ(alarms[0].rule_id, 1);
    EXPECT_FALSE(alarms[0].acknowledged);
}
