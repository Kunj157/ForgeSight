#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>

#include "ui/demo/demo_backend.h"
#include "ui/demo/demo_feed.h"

namespace {

constexpr std::int64_t kStart = 1767225600; // 2026-01-01T00:00:00Z

} // namespace

class DemoBackendTest : public ::testing::Test {
  protected:
    static void SetUpTestSuite() {
        static int argc = 0;
        static char* argv[] = {nullptr};
        static QCoreApplication app(argc, argv);
    }
};

TEST_F(DemoBackendTest, FetchRulesEmitsTheDefaultRuleSet) {
    ui::demo::DemoSimulator sim(1, kStart, 0.0);
    ui::demo::DemoBackend backend(sim);
    QSignalSpy rules(&backend, &ui::BackendApi::ruleReceived);
    QSignalSpy done(&backend, &ui::BackendApi::rulesLoadFinished);

    backend.fetchRules();
    ASSERT_TRUE(done.wait(2000));
    EXPECT_EQ(rules.count(), 8);
    EXPECT_TRUE(done.first().at(0).toBool());
}

TEST_F(DemoBackendTest, AcknowledgeThroughBackendMarksAlarm) {
    ui::demo::DemoSimulator sim(1, kStart, 0.0);
    const auto fired = sim.ingest("pump-001", "pressure", 0.8, "2026-01-01T00:00:00Z");
    ASSERT_EQ(fired.size(), 1u);

    ui::demo::DemoBackend backend(sim);
    QSignalSpy acks(&backend, &ui::BackendApi::alarmAckSucceeded);
    backend.acknowledgeAlarm(fired[0].id);
    ASSERT_TRUE(acks.wait(2000));
    EXPECT_TRUE(sim.alarms().front().acknowledged);
}

TEST_F(DemoBackendTest, FeedPublishesReadingsAsBackendJson) {
    ui::demo::DemoSimulator sim(1, kStart, 1.0);
    ui::demo::DemoFeed feed(sim);
    QSignalSpy connected(&feed, &ui::LiveFeed::connectedChanged);
    QSignalSpy readings(&feed, &ui::LiveFeed::readingReceived);

    feed.connectToServer();
    EXPECT_TRUE(feed.is_connected());
    ASSERT_TRUE(readings.wait(3000));

    const auto obj = QJsonDocument::fromJson(readings.first().first().toString().toUtf8()).object();
    EXPECT_EQ(obj.value(QStringLiteral("device_id")).toString(), QStringLiteral("pump-001"));
    EXPECT_TRUE(obj.contains(QStringLiteral("anomaly")));
    EXPECT_TRUE(obj.contains(QStringLiteral("timestamp")));

    feed.disconnectFromServer();
    EXPECT_FALSE(feed.is_connected());
    EXPECT_EQ(connected.count(), 2);
}
