#include <gtest/gtest.h>

#include <QDateTime>
#include <QRegularExpression>

#include "ui/time_format.h"

using ui::TimeFormat;

TEST(TimeFormatTest, ParsesIsoTWithOffset) {
    QDateTime dt = TimeFormat::parse("2026-09-20T08:32:32.540874+00:00");
    ASSERT_TRUE(dt.isValid());
    EXPECT_EQ(dt.toUTC().toString("yyyy-MM-dd HH:mm:ss"), "2026-09-20 08:32:32");
}

TEST(TimeFormatTest, ParsesPostgresSpaceFormWithShortOffset) {
    // Postgres timestamptz::text: space separator, microseconds, "+02" offset.
    QDateTime dt = TimeFormat::parse("2026-09-20 10:32:32.540874+02");
    ASSERT_TRUE(dt.isValid());
    EXPECT_EQ(dt.toUTC().toString("yyyy-MM-dd HH:mm:ss"), "2026-09-20 08:32:32");
}

TEST(TimeFormatTest, ParseRejectsGarbage) {
    EXPECT_FALSE(TimeFormat::parse("").isValid());
    EXPECT_FALSE(TimeFormat::parse("not-a-date").isValid());
}

TEST(TimeFormatTest, ClockTimeIsHmsOrEmpty) {
    TimeFormat fmt;
    QString t = fmt.clockTime("2026-09-20T08:32:32.540874+00:00");
    QRegularExpression re("^\\d{2}:\\d{2}:\\d{2}$");
    EXPECT_TRUE(re.match(t).hasMatch()) << t.toStdString();
    EXPECT_TRUE(fmt.clockTime("garbage").isEmpty());
}

TEST(TimeFormatTest, RelativeFromBuckets) {
    TimeFormat fmt;
    const QString base = "2026-09-20T08:00:00+00:00";
    QDateTime t0 = TimeFormat::parse(base);
    ASSERT_TRUE(t0.isValid());

    EXPECT_EQ(fmt.relativeFrom(base, t0.addSecs(1)), "just now");
    EXPECT_EQ(fmt.relativeFrom(base, t0.addSecs(30)), "30s ago");
    EXPECT_EQ(fmt.relativeFrom(base, t0.addSecs(5 * 60)), "5m ago");
    EXPECT_EQ(fmt.relativeFrom(base, t0.addSecs(3 * 3600)), "3h ago");
    EXPECT_EQ(fmt.relativeFrom(base, t0.addSecs(2 * 86400)), "2d ago");
}

TEST(TimeFormatTest, RelativeFromClampsFutureToJustNow) {
    TimeFormat fmt;
    const QString base = "2026-09-20T08:00:00+00:00";
    QDateTime t0 = TimeFormat::parse(base);
    // Reference time slightly before the timestamp (clock skew) -> not negative.
    EXPECT_EQ(fmt.relativeFrom(base, t0.addSecs(-5)), "just now");
}

TEST(TimeFormatTest, RelativeReturnsEmptyForGarbage) {
    TimeFormat fmt;
    EXPECT_TRUE(fmt.relative("").isEmpty());
    EXPECT_TRUE(fmt.relative("nonsense").isEmpty());
}

TEST(TimeFormatTest, DateTimeLabelIsCompactOrEmpty) {
    TimeFormat fmt;
    QString label = fmt.dateTimeLabel("2026-09-20T08:32:00+00:00");
    // Compact "MMM d, HH:mm" form — assert loosely to stay locale/TZ robust.
    EXPECT_FALSE(label.isEmpty());
    EXPECT_TRUE(label.contains(", "));
    QRegularExpression re("\\d{2}:\\d{2}$");
    EXPECT_TRUE(re.match(label).hasMatch()) << label.toStdString();

    EXPECT_TRUE(fmt.dateTimeLabel("").isEmpty());
    EXPECT_TRUE(fmt.dateTimeLabel("garbage").isEmpty());
}
