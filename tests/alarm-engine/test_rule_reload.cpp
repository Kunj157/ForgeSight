#include <gtest/gtest.h>

#include <optional>
#include <vector>

#include "alarm-engine/rule_reload.h"
#include "alarm-engine/types.h"

using namespace alarm_engine;

namespace {

Rule rule_with_threshold(double threshold) {
    Rule r;
    r.id = 7;
    r.device_id = "pump-001";
    r.sensor = "temperature";
    r.condition = Condition::GreaterThan;
    r.threshold = threshold;
    r.severity = Severity::Critical;
    return r;
}

} // namespace

TEST(RuleReloadTest, FreshLoadReplacesPreviousRules) {
    auto previous = std::vector<Rule>{rule_with_threshold(80.0)};
    auto loaded = std::vector<Rule>{rule_with_threshold(50.0)};

    auto rules = rules_for_poll(std::move(loaded), std::move(previous));

    ASSERT_EQ(rules.size(), 1u);
    EXPECT_DOUBLE_EQ(rules[0].threshold, 50.0);
}

TEST(RuleReloadTest, EmptyLoadClearsRules) {
    auto previous = std::vector<Rule>{rule_with_threshold(80.0)};

    auto rules = rules_for_poll(std::vector<Rule>{}, std::move(previous));

    EXPECT_TRUE(rules.empty());
}

TEST(RuleReloadTest, FailedLoadKeepsPreviousRules) {
    auto previous = std::vector<Rule>{rule_with_threshold(80.0)};

    auto rules = rules_for_poll(std::nullopt, std::move(previous));

    ASSERT_EQ(rules.size(), 1u);
    EXPECT_DOUBLE_EQ(rules[0].threshold, 80.0);
    EXPECT_EQ(rules[0].device_id, "pump-001");
}
