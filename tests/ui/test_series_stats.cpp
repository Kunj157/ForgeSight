#include <gtest/gtest.h>

#include <QVariantList>
#include <QVariantMap>

#include "ui/series_stats.h"

using ui::SeriesStats;

namespace {
QVariantList series(std::initializer_list<double> vals) {
    QVariantList list;
    for (double v : vals)
        list.push_back(v);
    return list;
}
} // namespace

TEST(SeriesStatsTest, EmptyIsInvalid) {
    SeriesStats s;
    QVariantMap m = s.compute(QVariantList{});
    EXPECT_FALSE(m.value("valid").toBool());
}

TEST(SeriesStatsTest, SinglePointIsFlat) {
    SeriesStats s;
    QVariantMap m = s.compute(series({5.0}));
    ASSERT_TRUE(m.value("valid").toBool());
    EXPECT_DOUBLE_EQ(m.value("min").toDouble(), 5.0);
    EXPECT_DOUBLE_EQ(m.value("max").toDouble(), 5.0);
    EXPECT_DOUBLE_EQ(m.value("last").toDouble(), 5.0);
    EXPECT_DOUBLE_EQ(m.value("delta").toDouble(), 0.0);
    EXPECT_EQ(m.value("direction").toString(), "flat");
}

TEST(SeriesStatsTest, RisingSeries) {
    SeriesStats s;
    QVariantMap m = s.compute(series({5.0, 7.5}));
    ASSERT_TRUE(m.value("valid").toBool());
    EXPECT_DOUBLE_EQ(m.value("min").toDouble(), 5.0);
    EXPECT_DOUBLE_EQ(m.value("max").toDouble(), 7.5);
    EXPECT_DOUBLE_EQ(m.value("last").toDouble(), 7.5);
    // delta compares the last value to the previous sample.
    EXPECT_DOUBLE_EQ(m.value("delta").toDouble(), 2.5);
    EXPECT_EQ(m.value("direction").toString(), "up");
}

TEST(SeriesStatsTest, FallingSeriesUsesLastTwoPoints) {
    SeriesStats s;
    QVariantMap m = s.compute(series({5.0, 9.0, 3.0}));
    ASSERT_TRUE(m.value("valid").toBool());
    EXPECT_DOUBLE_EQ(m.value("min").toDouble(), 3.0);
    EXPECT_DOUBLE_EQ(m.value("max").toDouble(), 9.0);
    EXPECT_DOUBLE_EQ(m.value("last").toDouble(), 3.0);
    EXPECT_DOUBLE_EQ(m.value("delta").toDouble(), -6.0);
    EXPECT_EQ(m.value("direction").toString(), "down");
}

TEST(SeriesStatsTest, EqualLastTwoIsFlat) {
    SeriesStats s;
    QVariantMap m = s.compute(series({1.0, 4.0, 4.0}));
    ASSERT_TRUE(m.value("valid").toBool());
    EXPECT_DOUBLE_EQ(m.value("delta").toDouble(), 0.0);
    EXPECT_EQ(m.value("direction").toString(), "flat");
    // range still reflects the whole window.
    EXPECT_DOUBLE_EQ(m.value("min").toDouble(), 1.0);
    EXPECT_DOUBLE_EQ(m.value("max").toDouble(), 4.0);
}

TEST(SeriesStatsTest, IgnoresNonNumericEntries) {
    SeriesStats s;
    QVariantList list;
    list.push_back(2.0);
    list.push_back(QString("oops"));
    list.push_back(6.0);
    QVariantMap m = s.compute(list);
    ASSERT_TRUE(m.value("valid").toBool());
    EXPECT_DOUBLE_EQ(m.value("min").toDouble(), 2.0);
    EXPECT_DOUBLE_EQ(m.value("max").toDouble(), 6.0);
    EXPECT_DOUBLE_EQ(m.value("last").toDouble(), 6.0);
}
