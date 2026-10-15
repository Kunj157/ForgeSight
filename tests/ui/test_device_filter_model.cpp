#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QSignalSpy>

#include "ui/device_filter_model.h"
#include "ui/device_model.h"

using ui::DeviceFilterModel;
using ui::DeviceModel;

class DeviceFilterModelTest : public ::testing::Test {
  protected:
    static void SetUpTestSuite() {
        static int argc = 0;
        static char* argv[] = {nullptr};
        static QCoreApplication app(argc, argv);
    }

    void SetUp() override {
        devices.updateDeviceMeta("pump-001", "Plant A", "Floor 1");
        devices.updateDeviceMeta("press-002", "Plant A", "Floor 2");
        devices.updateDeviceMeta("oven-003", "Plant B", "Floor 1");
        devices.updateDevice("pump-001", "temperature", 70.0, "C", "t", false);
        devices.updateDevice("pump-001", "pressure", 4.0, "bar", "t", true);
        devices.updateDevice("press-002", "vibration", 2.0, "mm/s", "t", false);
        devices.updateDevice("oven-003", "temperature", 180.0, "C", "t", true);
        filter.setSourceDevices(&devices);
    }

    QString id_at(int row) const {
        return filter.data(filter.index(row, 0), DeviceModel::DeviceIdRole).toString();
    }

    DeviceModel devices;
    DeviceFilterModel filter;
};

TEST_F(DeviceFilterModelTest, NoFilterPassesEverythingThrough) {
    EXPECT_EQ(filter.count(), 4);
    EXPECT_EQ(filter.criticalCount(), 2);
}

TEST_F(DeviceFilterModelTest, ExposesTheSourceRoleNames) {
    EXPECT_EQ(filter.roleNames(), devices.roleNames());
}

TEST_F(DeviceFilterModelTest, FiltersByPlantAndFloor) {
    filter.setPlant("Plant A");
    EXPECT_EQ(filter.count(), 3);
    filter.setFloor("Floor 1");
    EXPECT_EQ(filter.count(), 2);
    EXPECT_EQ(id_at(0), "pump-001");
    EXPECT_EQ(filter.criticalCount(), 1);
}

TEST_F(DeviceFilterModelTest, SearchMatchesDeviceOrSensorCaseInsensitively) {
    filter.setSearch("PRESS");
    // press-002 by id, pump-001/pressure by sensor
    EXPECT_EQ(filter.count(), 2);
    filter.setSearch("oven");
    EXPECT_EQ(filter.count(), 1);
    EXPECT_EQ(id_at(0), "oven-003");
    filter.setSearch("");
    EXPECT_EQ(filter.count(), 4);
}

TEST_F(DeviceFilterModelTest, LiveUpdatesFlowThroughWithoutResetting) {
    filter.setPlant("Plant B");
    QSignalSpy resets(&filter, &QAbstractItemModel::modelReset);
    QSignalSpy changed(&filter, &QAbstractItemModel::dataChanged);
    devices.updateDevice("oven-003", "temperature", 181.0, "C", "t2", false);
    EXPECT_EQ(resets.count(), 0);
    EXPECT_EQ(changed.count(), 1);
    EXPECT_DOUBLE_EQ(filter.data(filter.index(0, 0), DeviceModel::ValueRole).toDouble(), 181.0);
    EXPECT_EQ(filter.criticalCount(), 0);
}

TEST_F(DeviceFilterModelTest, NewDevicesInTheFilterAreAppended) {
    filter.setPlant("Plant B");
    QSignalSpy counts(&filter, &DeviceFilterModel::countChanged);
    devices.updateDeviceMeta("oven-009", "Plant B", "Floor 3");
    devices.updateDevice("oven-009", "temperature", 90.0, "C", "t", false);
    EXPECT_EQ(filter.count(), 2);
    EXPECT_GE(counts.count(), 1);
}

TEST_F(DeviceFilterModelTest, MetadataChangesMoveRowsBetweenFilters) {
    filter.setPlant("Plant B");
    devices.updateDeviceMeta("press-002", "Plant B", "Floor 1");
    EXPECT_EQ(filter.count(), 2);
}

TEST_F(DeviceFilterModelTest, SettersNotifyOnlyOnChange) {
    QSignalSpy plant(&filter, &DeviceFilterModel::plantChanged);
    filter.setPlant("Plant A");
    filter.setPlant("Plant A");
    EXPECT_EQ(plant.count(), 1);
}

TEST_F(DeviceFilterModelTest, CriticalCountChangeIsSignalledOncePerBurst) {
    QSignalSpy spy(&filter, &DeviceFilterModel::criticalCountChanged);
    QCoreApplication::processEvents();
    spy.clear();
    for (int i = 0; i < 5; ++i)
        devices.updateDevice("press-002", "vibration", 9.0 + i, "mm/s", "t", true);
    QCoreApplication::processEvents();
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(filter.criticalCount(), 3);
}
