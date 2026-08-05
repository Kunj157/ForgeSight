#include <gtest/gtest.h>

#include <QAbstractItemModel>
#include <QCoreApplication>
#include <QFileInfo>
#include <QSignalSpy>

#include "ui/alarm_model.h"
#include "ui/device_model.h"
#include "ui/history_model.h"

class DeviceModelTest : public ::testing::Test {
  protected:
    static void SetUpTestSuite() {
        static int argc = 0;
        static char* argv[] = {nullptr};
        static QCoreApplication app(argc, argv);
    }
};

TEST_F(DeviceModelTest, EmptyModelHasZeroRows) {
    ui::DeviceModel model;
    EXPECT_EQ(model.rowCount(), 0);
}

TEST_F(DeviceModelTest, RoleNamesAreCorrect) {
    ui::DeviceModel model;
    auto roles = model.roleNames();
    EXPECT_TRUE(roles.contains(ui::DeviceModel::DeviceIdRole));
    EXPECT_TRUE(roles.contains(ui::DeviceModel::SensorRole));
    EXPECT_TRUE(roles.contains(ui::DeviceModel::ValueRole));
    EXPECT_TRUE(roles.contains(ui::DeviceModel::UnitRole));
    EXPECT_TRUE(roles.contains(ui::DeviceModel::TimestampRole));
    EXPECT_TRUE(roles.contains(ui::DeviceModel::AnomalyRole));
    EXPECT_TRUE(roles.contains(ui::DeviceModel::StatusRole));
}

TEST_F(DeviceModelTest, UpdateDeviceAddsNewEntry) {
    ui::DeviceModel model;
    model.updateDevice("pump-001", "temperature", 65.0, "°C", "2026-07-26T10:00:00Z", false);
    EXPECT_EQ(model.rowCount(), 1);

    auto idx = model.index(0);
    EXPECT_EQ(model.data(idx, ui::DeviceModel::DeviceIdRole).toString(), "pump-001");
    EXPECT_EQ(model.data(idx, ui::DeviceModel::SensorRole).toString(), "temperature");
    EXPECT_DOUBLE_EQ(model.data(idx, ui::DeviceModel::ValueRole).toDouble(), 65.0);
}

TEST_F(DeviceModelTest, UpdateDeviceUpdatesExistingEntry) {
    ui::DeviceModel model;
    model.updateDevice("pump-001", "temperature", 65.0, "°C", "2026-07-26T10:00:00Z", false);
    model.updateDevice("pump-001", "temperature", 80.0, "°C", "2026-07-26T10:01:00Z", false);
    EXPECT_EQ(model.rowCount(), 1);

    auto idx = model.index(0);
    EXPECT_DOUBLE_EQ(model.data(idx, ui::DeviceModel::ValueRole).toDouble(), 80.0);
}

TEST_F(DeviceModelTest, MultipleDevicesTracked) {
    ui::DeviceModel model;
    model.updateDevice("pump-001", "temperature", 65.0, "°C", "2026-07-26T10:00:00Z", false);
    model.updateDevice("pump-002", "pressure", 2.5, "bar", "2026-07-26T10:00:00Z", false);
    EXPECT_EQ(model.rowCount(), 2);
}

TEST_F(DeviceModelTest, AnomalyDeviceMarkedCritical) {
    ui::DeviceModel model;
    model.updateDevice("pump-001", "temperature", 120.0, "°C", "2026-07-26T10:00:00Z", true);
    auto idx = model.index(0);
    EXPECT_EQ(model.data(idx, ui::DeviceModel::StatusRole).toString(), "critical");
    EXPECT_TRUE(model.data(idx, ui::DeviceModel::AnomalyRole).toBool());
}

TEST_F(DeviceModelTest, NormalDeviceMarkedNormal) {
    ui::DeviceModel model;
    model.updateDevice("pump-001", "temperature", 65.0, "°C", "2026-07-26T10:00:00Z", false);
    auto idx = model.index(0);
    EXPECT_EQ(model.data(idx, ui::DeviceModel::StatusRole).toString(), "normal");
}

TEST_F(DeviceModelTest, ClearRemovesAllDevices) {
    ui::DeviceModel model;
    model.updateDevice("pump-001", "temperature", 65.0, "°C", "2026-07-26T10:00:00Z", false);
    model.updateDevice("pump-002", "pressure", 2.5, "bar", "2026-07-26T10:00:00Z", false);
    model.clear();
    EXPECT_EQ(model.rowCount(), 0);
}

TEST_F(DeviceModelTest, DataChangedSignalEmittedOnUpdate) {
    ui::DeviceModel model;
    model.updateDevice("pump-001", "temperature", 65.0, "°C", "2026-07-26T10:00:00Z", false);
    QSignalSpy spy(&model, &QAbstractItemModel::dataChanged);
    model.updateDevice("pump-001", "temperature", 80.0, "°C", "2026-07-26T10:01:00Z", false);
    EXPECT_EQ(spy.count(), 1);
}

TEST_F(DeviceModelTest, DeviceStatusMethod) {
    ui::DeviceModel model;
    model.updateDevice("pump-001", "temperature", 65.0, "°C", "2026-07-26T10:00:00Z", false);
    EXPECT_EQ(model.device_status("pump-001"), "normal");
    EXPECT_EQ(model.device_status("nonexistent"), "");
}

// ---- DeviceModel plant/floor hierarchy ----

TEST_F(DeviceModelTest, DeviceDefaultsToUnassignedPlantAndFloor) {
    ui::DeviceModel model;
    model.updateDevice("pump-001", "temperature", 65.0, "°C", "2026-07-26T10:00:00Z", false);
    auto idx = model.index(0);
    EXPECT_EQ(model.data(idx, ui::DeviceModel::PlantRole).toString(), "Unassigned");
    EXPECT_EQ(model.data(idx, ui::DeviceModel::FloorRole).toString(), "Unassigned");
}

TEST_F(DeviceModelTest, UpdateDeviceMetaBeforeReadingIsApplied) {
    ui::DeviceModel model;
    model.updateDeviceMeta("pump-001", "Plant A", "Floor 1");
    model.updateDevice("pump-001", "temperature", 65.0, "°C", "2026-07-26T10:00:00Z", false);
    auto idx = model.index(0);
    EXPECT_EQ(model.data(idx, ui::DeviceModel::PlantRole).toString(), "Plant A");
    EXPECT_EQ(model.data(idx, ui::DeviceModel::FloorRole).toString(), "Floor 1");
}

TEST_F(DeviceModelTest, UpdateDeviceMetaAfterReadingUpdatesRetroactively) {
    ui::DeviceModel model;
    model.updateDevice("pump-001", "temperature", 65.0, "°C", "2026-07-26T10:00:00Z", false);
    model.updateDevice("pump-001", "pressure", 2.5, "bar", "2026-07-26T10:00:00Z", false);
    model.updateDeviceMeta("pump-001", "Plant A", "Floor 1");

    for (int i = 0; i < model.rowCount(); ++i) {
        auto idx = model.index(i);
        EXPECT_EQ(model.data(idx, ui::DeviceModel::PlantRole).toString(), "Plant A");
        EXPECT_EQ(model.data(idx, ui::DeviceModel::FloorRole).toString(), "Floor 1");
    }
}

TEST_F(DeviceModelTest, UpdateDeviceMetaEmitsDataChangedForExistingRows) {
    ui::DeviceModel model;
    model.updateDevice("pump-001", "temperature", 65.0, "°C", "2026-07-26T10:00:00Z", false);
    QSignalSpy spy(&model, &QAbstractItemModel::dataChanged);
    model.updateDeviceMeta("pump-001", "Plant A", "Floor 1");
    EXPECT_EQ(spy.count(), 1);
}

TEST_F(DeviceModelTest, PlantsReturnsSortedUniquePlants) {
    ui::DeviceModel model;
    model.updateDeviceMeta("compressor-001", "Plant B", "Floor 1");
    model.updateDeviceMeta("pump-001", "Plant A", "Floor 1");
    model.updateDeviceMeta("pump-002", "Plant A", "Floor 2");
    model.updateDevice("pump-001", "temperature", 65.0, "°C", "2026-07-26T10:00:00Z", false);
    model.updateDevice("pump-002", "temperature", 65.0, "°C", "2026-07-26T10:00:00Z", false);
    model.updateDevice("compressor-001", "temperature", 65.0, "°C", "2026-07-26T10:00:00Z", false);

    auto plants = model.plants();
    ASSERT_EQ(plants.size(), 2);
    EXPECT_EQ(plants[0], "Plant A");
    EXPECT_EQ(plants[1], "Plant B");
}

TEST_F(DeviceModelTest, FloorsReturnsSortedUniqueFloorsWithinPlant) {
    ui::DeviceModel model;
    model.updateDeviceMeta("pump-001", "Plant A", "Floor 2");
    model.updateDeviceMeta("pump-002", "Plant A", "Floor 1");
    model.updateDeviceMeta("compressor-001", "Plant B", "Floor 9");
    model.updateDevice("pump-001", "temperature", 65.0, "°C", "2026-07-26T10:00:00Z", false);
    model.updateDevice("pump-002", "temperature", 65.0, "°C", "2026-07-26T10:00:00Z", false);
    model.updateDevice("compressor-001", "temperature", 65.0, "°C", "2026-07-26T10:00:00Z", false);

    auto floors = model.floors("Plant A");
    ASSERT_EQ(floors.size(), 2);
    EXPECT_EQ(floors[0], "Floor 1");
    EXPECT_EQ(floors[1], "Floor 2");
}

TEST_F(DeviceModelTest, DevicesForReturnsOnlyMatchingPlantAndFloor) {
    ui::DeviceModel model;
    model.updateDeviceMeta("pump-001", "Plant A", "Floor 1");
    model.updateDeviceMeta("compressor-001", "Plant B", "Floor 1");
    model.updateDevice("pump-001", "temperature", 65.0, "°C", "2026-07-26T10:00:00Z", false);
    model.updateDevice("compressor-001", "temperature", 72.0, "°C", "2026-07-26T10:00:00Z", false);

    auto rows = model.devicesFor("Plant A", "Floor 1");
    ASSERT_EQ(rows.size(), 1);
    auto row = rows[0].toMap();
    EXPECT_EQ(row["deviceId"].toString(), "pump-001");
    EXPECT_DOUBLE_EQ(row["value"].toDouble(), 65.0);
}

TEST_F(DeviceModelTest, DevicesForReturnsEmptyForUnknownGroup) {
    ui::DeviceModel model;
    model.updateDevice("pump-001", "temperature", 65.0, "°C", "2026-07-26T10:00:00Z", false);
    EXPECT_TRUE(model.devicesFor("Nowhere", "Nothing").empty());
}

// ---- DeviceModel::deviceUpdated (offline cache persistence hook) ----

TEST_F(DeviceModelTest, UpdateDeviceEmitsDeviceUpdatedWithCurrentPlantFloor) {
    ui::DeviceModel model;
    model.updateDeviceMeta("pump-001", "Plant A", "Floor 1");

    QSignalSpy spy(&model, &ui::DeviceModel::deviceUpdated);
    model.updateDevice("pump-001", "temperature", 65.0, "°C", "2026-07-26T10:00:00Z", false);

    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy[0][0].toString(), "pump-001");
    EXPECT_EQ(spy[0][1].toString(), "temperature");
    EXPECT_DOUBLE_EQ(spy[0][2].toDouble(), 65.0);
    EXPECT_EQ(spy[0][6].toString(), "Plant A");
    EXPECT_EQ(spy[0][7].toString(), "Floor 1");
}

TEST_F(DeviceModelTest, UpdateDeviceEmitsDeviceUpdatedOnSubsequentUpdatesToo) {
    ui::DeviceModel model;
    model.updateDevice("pump-001", "temperature", 65.0, "°C", "2026-07-26T10:00:00Z", false);

    QSignalSpy spy(&model, &ui::DeviceModel::deviceUpdated);
    model.updateDevice("pump-001", "temperature", 80.0, "°C", "2026-07-26T10:01:00Z", false);

    ASSERT_EQ(spy.count(), 1);
    EXPECT_DOUBLE_EQ(spy[0][2].toDouble(), 80.0);
}

// ---- AlarmModel Tests ----

class AlarmModelTest : public ::testing::Test {
  protected:
    static void SetUpTestSuite() {
        static int argc = 0;
        static char* argv[] = {nullptr};
        static QCoreApplication app(argc, argv);
    }
};

TEST_F(AlarmModelTest, EmptyModelHasZeroRows) {
    ui::AlarmModel model;
    EXPECT_EQ(model.rowCount(), 0);
}

TEST_F(AlarmModelTest, AddAlarmIncreasesCount) {
    ui::AlarmModel model;
    model.add_alarm(1, "pump-001", "temperature", 95.0, "warning", "High temp",
                    "2026-07-26T10:00:00Z");
    EXPECT_EQ(model.rowCount(), 1);
}

TEST_F(AlarmModelTest, AlarmDataIsCorrect) {
    ui::AlarmModel model;
    model.add_alarm(1, "pump-001", "temperature", 95.0, "warning", "High temp",
                    "2026-07-26T10:00:00Z");
    auto idx = model.index(0);
    EXPECT_EQ(model.data(idx, ui::AlarmModel::DeviceIdRole).toString(), "pump-001");
    EXPECT_EQ(model.data(idx, ui::AlarmModel::SeverityRole).toString(), "warning");
    EXPECT_DOUBLE_EQ(model.data(idx, ui::AlarmModel::ValueRole).toDouble(), 95.0);
    EXPECT_FALSE(model.data(idx, ui::AlarmModel::AcknowledgedRole).toBool());
}

TEST_F(AlarmModelTest, AcknowledgeAlarm) {
    ui::AlarmModel model;
    model.add_alarm(1, "pump-001", "temperature", 95.0, "warning", "High temp",
                    "2026-07-26T10:00:00Z");
    QSignalSpy spy(&model, &ui::AlarmModel::alarmAcknowledged);
    model.acknowledge(1);
    auto idx = model.index(0);
    EXPECT_TRUE(model.data(idx, ui::AlarmModel::AcknowledgedRole).toBool());
    EXPECT_EQ(spy.count(), 1);
}

TEST_F(AlarmModelTest, AcknowledgeNonexistentIsNoop) {
    ui::AlarmModel model;
    model.add_alarm(1, "pump-001", "temperature", 95.0, "warning", "High temp",
                    "2026-07-26T10:00:00Z");
    model.acknowledge(999);
    auto idx = model.index(0);
    EXPECT_FALSE(model.data(idx, ui::AlarmModel::AcknowledgedRole).toBool());
}

TEST_F(AlarmModelTest, UnacknowledgedCount) {
    ui::AlarmModel model;
    model.add_alarm(1, "pump-001", "temperature", 95.0, "warning", "High temp",
                    "2026-07-26T10:00:00Z");
    model.add_alarm(2, "pump-001", "temperature", 99.0, "critical", "Very high",
                    "2026-07-26T10:01:00Z");
    EXPECT_EQ(model.unacknowledged_count(), 2);
    model.acknowledge(1);
    EXPECT_EQ(model.unacknowledged_count(), 1);
}

TEST_F(AlarmModelTest, ClearRemovesAllAlarms) {
    ui::AlarmModel model;
    model.add_alarm(1, "pump-001", "temperature", 95.0, "warning", "High temp",
                    "2026-07-26T10:00:00Z");
    model.clear();
    EXPECT_EQ(model.rowCount(), 0);
}

TEST_F(AlarmModelTest, AlarmAddedSignal) {
    ui::AlarmModel model;
    QSignalSpy spy(&model, &ui::AlarmModel::alarmAdded);
    model.add_alarm(1, "pump-001", "temperature", 95.0, "warning", "High temp",
                    "2026-07-26T10:00:00Z");
    EXPECT_EQ(spy.count(), 1);
}

TEST_F(AlarmModelTest, DuplicateIdUpdatesInsteadOfInsert) {
    ui::AlarmModel model;
    model.add_alarm(1, "pump-001", "temperature", 95.0, "warning", "High temp",
                    "2026-07-26T10:00:00Z", false);
    model.add_alarm(1, "pump-001", "temperature", 99.0, "critical", "Updated",
                    "2026-07-26T10:01:00Z", true);
    EXPECT_EQ(model.rowCount(), 1);
    auto idx = model.index(0);
    EXPECT_DOUBLE_EQ(model.data(idx, ui::AlarmModel::ValueRole).toDouble(), 99.0);
    EXPECT_TRUE(model.data(idx, ui::AlarmModel::AcknowledgedRole).toBool());
    EXPECT_EQ(model.unacknowledged_count(), 0);
}

// ---- HistoryModel Tests ----

class HistoryModelTest : public ::testing::Test {
  protected:
    static void SetUpTestSuite() {
        static int argc = 0;
        static char* argv[] = {nullptr};
        static QCoreApplication app(argc, argv);
    }
};

TEST_F(HistoryModelTest, EmptyModelHasZeroRows) {
    ui::HistoryModel model;
    EXPECT_EQ(model.rowCount(), 0);
    EXPECT_EQ(model.point_count(), 0);
}

TEST_F(HistoryModelTest, AddPointIncreasesCount) {
    ui::HistoryModel model;
    model.add_point("pump-001", "temperature", 65.0, "°C",
                    QDateTime::fromString("2026-07-26T10:00:00Z", Qt::ISODate), false);
    EXPECT_EQ(model.rowCount(), 1);
}

TEST_F(HistoryModelTest, PointDataIsCorrect) {
    ui::HistoryModel model;
    model.add_point("pump-001", "temperature", 65.0, "°C",
                    QDateTime::fromString("2026-07-26T10:00:00Z", Qt::ISODate), false);
    auto idx = model.index(0);
    EXPECT_EQ(model.data(idx, ui::HistoryModel::DeviceIdRole).toString(), "pump-001");
    EXPECT_EQ(model.data(idx, ui::HistoryModel::SensorRole).toString(), "temperature");
    EXPECT_DOUBLE_EQ(model.data(idx, ui::HistoryModel::ValueRole).toDouble(), 65.0);
    EXPECT_EQ(model.data(idx, ui::HistoryModel::UnitRole).toString(), "°C");
    EXPECT_FALSE(model.data(idx, ui::HistoryModel::AnomalyRole).toBool());
}

TEST_F(HistoryModelTest, AnomalyPoint) {
    ui::HistoryModel model;
    model.add_point("pump-001", "temperature", 120.0, "°C",
                    QDateTime::fromString("2026-07-26T10:00:00Z", Qt::ISODate), true);
    auto idx = model.index(0);
    EXPECT_TRUE(model.data(idx, ui::HistoryModel::AnomalyRole).toBool());
}

TEST_F(HistoryModelTest, MultiplePoints) {
    ui::HistoryModel model;
    model.add_point("pump-001", "temperature", 65.0, "°C",
                    QDateTime::fromString("2026-07-26T10:00:00Z", Qt::ISODate), false);
    model.add_point("pump-001", "temperature", 70.0, "°C",
                    QDateTime::fromString("2026-07-26T10:01:00Z", Qt::ISODate), false);
    model.add_point("pump-001", "temperature", 68.0, "°C",
                    QDateTime::fromString("2026-07-26T10:02:00Z", Qt::ISODate), false);
    EXPECT_EQ(model.rowCount(), 3);
}

TEST_F(HistoryModelTest, ClearRemovesAll) {
    ui::HistoryModel model;
    model.add_point("pump-001", "temperature", 65.0, "°C",
                    QDateTime::fromString("2026-07-26T10:00:00Z", Qt::ISODate), false);
    model.clear();
    EXPECT_EQ(model.rowCount(), 0);
}

TEST_F(HistoryModelTest, RoleNamesAreCorrect) {
    ui::HistoryModel model;
    auto roles = model.roleNames();
    EXPECT_TRUE(roles.contains(ui::HistoryModel::DeviceIdRole));
    EXPECT_TRUE(roles.contains(ui::HistoryModel::SensorRole));
    EXPECT_TRUE(roles.contains(ui::HistoryModel::ValueRole));
    EXPECT_TRUE(roles.contains(ui::HistoryModel::UnitRole));
    EXPECT_TRUE(roles.contains(ui::HistoryModel::TimestampRole));
    EXPECT_TRUE(roles.contains(ui::HistoryModel::AnomalyRole));
}

TEST_F(HistoryModelTest, PointCountChangedEmittedOnAddAndClear) {
    ui::HistoryModel model;
    QSignalSpy spy(&model, &ui::HistoryModel::pointCountChanged);

    model.add_point("pump-001", "temperature", 65.0, "°C",
                    QDateTime::fromString("2026-07-26T10:00:00Z", Qt::ISODate), false);
    EXPECT_EQ(spy.count(), 1);

    model.clear();
    EXPECT_EQ(spy.count(), 2);
}

TEST_F(HistoryModelTest, DefaultExportPathIsNotCwdRelative) {
    ui::HistoryModel model;
    QString path = model.default_export_path("history_export.csv");

    EXPECT_TRUE(path.endsWith("/history_export.csv"));
    EXPECT_NE(path, "history_export.csv");

    QFileInfo info(path);
    EXPECT_TRUE(info.dir().exists());
}

TEST_F(HistoryModelTest, OutOfRangeIndexReturnsInvalid) {
    ui::HistoryModel model;
    EXPECT_TRUE(model.data(model.index(0), Qt::DisplayRole).isNull());
    EXPECT_TRUE(model.data(model.index(-1), Qt::DisplayRole).isNull());
}
