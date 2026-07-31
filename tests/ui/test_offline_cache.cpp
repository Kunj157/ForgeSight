#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>

#include "ui/offline_cache.h"

class OfflineCacheTest : public ::testing::Test {
  protected:
    // QtSql lazily constructs its own internal statics (driver registry
    // etc.) the first time QSqlDatabase::addDatabase() is called. If the
    // QCoreApplication outlives those (e.g. a function-local `static`
    // destroyed via atexit at real process exit), ~QCoreApplication's
    // qt_call_post_routines() can run after QtSql's statics are already
    // gone, crashing in QtSql's driver-registry cleanup. Owning the app via
    // a pointer we explicitly delete in TearDownTestSuite — well before
    // process exit — keeps teardown ordering deterministic.
    static void SetUpTestSuite() {
        static int argc = 0;
        static char* argv[] = {nullptr};
        app = new QCoreApplication(argc, argv);
    }

    static void TearDownTestSuite() {
        delete app;
        app = nullptr;
    }

    static QCoreApplication* app;

    void SetUp() override {
        ASSERT_TRUE(tempDir.isValid());
        dbPath = tempDir.filePath("cache.db");
    }

    QTemporaryDir tempDir;
    QString dbPath;
};

QCoreApplication* OfflineCacheTest::app = nullptr;

TEST_F(OfflineCacheTest, OpenCreatesDatabaseFile) {
    ui::OfflineCache cache;
    EXPECT_TRUE(cache.open(dbPath));
    EXPECT_TRUE(QFile::exists(dbPath));
}

TEST_F(OfflineCacheTest, LoadDeviceStatesEmptyWhenNothingSaved) {
    ui::OfflineCache cache;
    ASSERT_TRUE(cache.open(dbPath));
    EXPECT_TRUE(cache.loadDeviceStates().empty());
}

TEST_F(OfflineCacheTest, SaveThenLoadRoundTripsDeviceState) {
    ui::OfflineCache cache;
    ASSERT_TRUE(cache.open(dbPath));
    cache.saveDeviceState("pump-001", "temperature", 65.5, "°C", "2026-07-31T10:00:00Z", false,
                          "Plant A", "Floor 1");

    auto states = cache.loadDeviceStates();
    ASSERT_EQ(states.size(), 1u);
    EXPECT_EQ(states[0].device_id, "pump-001");
    EXPECT_EQ(states[0].sensor, "temperature");
    EXPECT_DOUBLE_EQ(states[0].value, 65.5);
    EXPECT_EQ(states[0].unit, "°C");
    EXPECT_EQ(states[0].timestamp, "2026-07-31T10:00:00Z");
    EXPECT_FALSE(states[0].anomaly);
    EXPECT_EQ(states[0].plant, "Plant A");
    EXPECT_EQ(states[0].floor, "Floor 1");
}

TEST_F(OfflineCacheTest, SavingSameDeviceSensorUpdatesInPlace) {
    ui::OfflineCache cache;
    ASSERT_TRUE(cache.open(dbPath));
    cache.saveDeviceState("pump-001", "temperature", 65.5, "°C", "2026-07-31T10:00:00Z", false,
                          "Plant A", "Floor 1");
    cache.saveDeviceState("pump-001", "temperature", 80.0, "°C", "2026-07-31T10:01:00Z", true,
                          "Plant A", "Floor 1");

    auto states = cache.loadDeviceStates();
    ASSERT_EQ(states.size(), 1u);
    EXPECT_DOUBLE_EQ(states[0].value, 80.0);
    EXPECT_TRUE(states[0].anomaly);
}

TEST_F(OfflineCacheTest, MultipleDevicesPersistIndependently) {
    ui::OfflineCache cache;
    ASSERT_TRUE(cache.open(dbPath));
    cache.saveDeviceState("pump-001", "temperature", 65.5, "°C", "t1", false, "Plant A", "Floor 1");
    cache.saveDeviceState("compressor-001", "current", 12.0, "A", "t2", false, "Plant A",
                          "Floor 2");

    EXPECT_EQ(cache.loadDeviceStates().size(), 2u);
}

TEST_F(OfflineCacheTest, StateSurvivesReopeningSameFile) {
    {
        ui::OfflineCache cache;
        ASSERT_TRUE(cache.open(dbPath));
        cache.saveDeviceState("pump-001", "temperature", 65.5, "°C", "t1", false, "Plant A",
                              "Floor 1");
    }
    {
        ui::OfflineCache cache;
        ASSERT_TRUE(cache.open(dbPath));
        auto states = cache.loadDeviceStates();
        ASSERT_EQ(states.size(), 1u);
        EXPECT_EQ(states[0].device_id, "pump-001");
    }
}

TEST_F(OfflineCacheTest, QueueAckAddsToPendingList) {
    ui::OfflineCache cache;
    ASSERT_TRUE(cache.open(dbPath));
    cache.queueAck(42);
    EXPECT_EQ(cache.pendingAcks(), QList<qint64>({42}));
    EXPECT_EQ(cache.pendingAckCount(), 1);
}

TEST_F(OfflineCacheTest, QueueAckIsIdempotent) {
    ui::OfflineCache cache;
    ASSERT_TRUE(cache.open(dbPath));
    cache.queueAck(42);
    cache.queueAck(42);
    EXPECT_EQ(cache.pendingAckCount(), 1);
}

TEST_F(OfflineCacheTest, ClearAckRemovesFromPendingList) {
    ui::OfflineCache cache;
    ASSERT_TRUE(cache.open(dbPath));
    cache.queueAck(42);
    cache.queueAck(43);
    cache.clearAck(42);

    EXPECT_EQ(cache.pendingAcks(), QList<qint64>({43}));
}

TEST_F(OfflineCacheTest, PendingAckCountChangedEmittedOnQueueAndClear) {
    ui::OfflineCache cache;
    ASSERT_TRUE(cache.open(dbPath));
    QSignalSpy spy(&cache, &ui::OfflineCache::pendingAckCountChanged);
    cache.queueAck(1);
    cache.clearAck(1);
    EXPECT_EQ(spy.count(), 2);
}

TEST_F(OfflineCacheTest, AckQueuePersistsAcrossReopen) {
    {
        ui::OfflineCache cache;
        ASSERT_TRUE(cache.open(dbPath));
        cache.queueAck(7);
    }
    {
        ui::OfflineCache cache;
        ASSERT_TRUE(cache.open(dbPath));
        EXPECT_EQ(cache.pendingAcks(), QList<qint64>({7}));
    }
}

TEST_F(OfflineCacheTest, DefaultPathIsNotCwdRelative) {
    QString path = ui::OfflineCache::default_path();
    EXPECT_TRUE(QDir::isAbsolutePath(path));
    EXPECT_NE(path, QDir::current().filePath("forgesight_cache.db"));
}

TEST_F(OfflineCacheTest, OperationsOnUnopenedCacheAreNoopsNotCrashes) {
    ui::OfflineCache cache;
    EXPECT_TRUE(cache.loadDeviceStates().empty());
    cache.saveDeviceState("a", "b", 1.0, "u", "t", false, "p", "f");
    cache.queueAck(1);
    EXPECT_TRUE(cache.pendingAcks().empty());
    EXPECT_EQ(cache.pendingAckCount(), 0);
}
