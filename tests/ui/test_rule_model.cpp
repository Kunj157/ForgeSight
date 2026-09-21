#include <gtest/gtest.h>

#include <QCoreApplication>

#include "ui/rule_model.h"

class RuleModelTest : public ::testing::Test {
  protected:
    static void SetUpTestSuite() {
        static int argc = 0;
        static char* argv[] = {nullptr};
        static QCoreApplication app(argc, argv);
    }
};

TEST_F(RuleModelTest, StartsEmpty) {
    ui::RuleModel m;
    EXPECT_EQ(m.count(), 0);
    EXPECT_EQ(m.rowCount(), 0);
}

TEST_F(RuleModelTest, AddRuleIncreasesCountAndExposesData) {
    ui::RuleModel m;
    m.add_rule(7, "pump-001", "temperature", "gt", 80.0, "warning");
    ASSERT_EQ(m.count(), 1);

    const auto idx = m.index(0, 0);
    EXPECT_EQ(m.data(idx, ui::RuleModel::IdRole).toLongLong(), 7);
    EXPECT_EQ(m.data(idx, ui::RuleModel::DeviceIdRole).toString(), "pump-001");
    EXPECT_EQ(m.data(idx, ui::RuleModel::SensorRole).toString(), "temperature");
    EXPECT_EQ(m.data(idx, ui::RuleModel::ConditionRole).toString(), "gt");
    EXPECT_DOUBLE_EQ(m.data(idx, ui::RuleModel::ThresholdRole).toDouble(), 80.0);
    EXPECT_EQ(m.data(idx, ui::RuleModel::SeverityRole).toString(), "warning");
}

TEST_F(RuleModelTest, RoleNamesExposeFieldsForQml) {
    ui::RuleModel m;
    const auto names = m.roleNames();
    EXPECT_EQ(names.value(ui::RuleModel::DeviceIdRole), QByteArray("deviceId"));
    EXPECT_EQ(names.value(ui::RuleModel::SensorRole), QByteArray("sensor"));
    EXPECT_EQ(names.value(ui::RuleModel::ConditionRole), QByteArray("condition"));
    EXPECT_EQ(names.value(ui::RuleModel::ThresholdRole), QByteArray("threshold"));
    EXPECT_EQ(names.value(ui::RuleModel::SeverityRole), QByteArray("severity"));
    EXPECT_EQ(names.value(ui::RuleModel::IdRole), QByteArray("ruleId"));
}

TEST_F(RuleModelTest, GetReturnsRowAsMap) {
    ui::RuleModel m;
    m.add_rule(3, "compressor-001", "current", "lt", 2.5, "critical");

    const auto map = m.get(0);
    EXPECT_EQ(map.value("ruleId").toLongLong(), 3);
    EXPECT_EQ(map.value("deviceId").toString(), "compressor-001");
    EXPECT_EQ(map.value("sensor").toString(), "current");
    EXPECT_EQ(map.value("condition").toString(), "lt");
    EXPECT_DOUBLE_EQ(map.value("threshold").toDouble(), 2.5);
    EXPECT_EQ(map.value("severity").toString(), "critical");

    // Out-of-range access returns an empty map rather than crashing.
    EXPECT_TRUE(m.get(5).isEmpty());
    EXPECT_TRUE(m.get(-1).isEmpty());
}

TEST_F(RuleModelTest, ClearRemovesAll) {
    ui::RuleModel m;
    m.add_rule(1, "a", "b", "gt", 1.0, "info");
    m.add_rule(2, "c", "d", "lt", 2.0, "warning");
    ASSERT_EQ(m.count(), 2);

    m.clear();
    EXPECT_EQ(m.count(), 0);
    EXPECT_EQ(m.rowCount(), 0);
}
