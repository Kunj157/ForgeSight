#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QSignalSpy>
#include <QTest>

#include "ui/device_model.h"
#include "ui/plant_layout.h"
#include "ui/plant_scene_model.h"

using ui::PlantDevice;
using ui::PlantLayout;

namespace {
QVector<PlantDevice> sample() {
    return {
        {"pump-001", "temperature", "Plant A", "Floor 1", 70.0, false},
        {"pump-002", "temperature", "Plant A", "Floor 1", 90.0, true},
        {"press-003", "pressure", "Plant A", "Floor 2", 4.0, false},
        {"oven-004", "temperature", "Plant B", "Floor 1", 80.0, false},
    };
}
} // namespace

TEST(PlantLayoutTest, OneZonePerPlantFloorWithCounts) {
    PlantLayout layout;
    auto scene = layout.layout(sample());
    ASSERT_EQ(scene.zones.size(), 3);
    EXPECT_EQ(scene.zones[0].plant, "Plant A");
    EXPECT_EQ(scene.zones[0].floor, "Floor 1");
    EXPECT_EQ(scene.zones[0].count, 2);
    EXPECT_EQ(scene.zones[0].critical, 1);
    EXPECT_EQ(scene.zones[2].plant, "Plant B");
}

TEST(PlantLayoutTest, EveryDeviceIsPlacedInsideItsZone) {
    PlantLayout layout;
    auto scene = layout.layout(sample());
    ASSERT_EQ(scene.devices.size(), 4);
    for (const auto& d : scene.devices) {
        const auto& z = scene.zones[d.zone];
        EXPECT_EQ(z.plant, d.plant);
        EXPECT_EQ(z.floor, d.floor);
        EXPECT_GE(d.x, z.x - z.width / 2);
        EXPECT_LE(d.x, z.x + z.width / 2);
        EXPECT_GE(d.z, z.z - z.depth / 2);
        EXPECT_LE(d.z, z.z + z.depth / 2);
    }
}

TEST(PlantLayoutTest, DevicesDoNotOverlap) {
    QVector<PlantDevice> many;
    for (int i = 0; i < 600; ++i)
        many.push_back({QString("dev-%1").arg(i), "temperature", QString("Plant %1").arg(i % 2),
                        QString("Floor %1").arg(i % 3), double(i), false});
    PlantLayout layout;
    auto scene = layout.layout(many);
    ASSERT_EQ(scene.devices.size(), 600);
    QSet<QPair<int, int>> cells;
    for (const auto& d : scene.devices)
        cells.insert({qRound(d.x * 10), qRound(d.z * 10)});
    EXPECT_EQ(cells.size(), 600);
}

TEST(PlantLayoutTest, SceneIsCenteredOnTheOrigin) {
    PlantLayout layout;
    auto scene = layout.layout(sample());
    float minX = 1e9f, maxX = -1e9f, minZ = 1e9f, maxZ = -1e9f;
    for (const auto& z : scene.zones) {
        minX = std::min(minX, z.x - z.width / 2);
        maxX = std::max(maxX, z.x + z.width / 2);
        minZ = std::min(minZ, z.z - z.depth / 2);
        maxZ = std::max(maxZ, z.z + z.depth / 2);
    }
    EXPECT_NEAR(minX + maxX, 0.0f, 0.01f);
    EXPECT_NEAR(minZ + maxZ, 0.0f, 0.01f);
    EXPECT_NEAR(scene.width, maxX - minX, 0.01f);
    EXPECT_NEAR(scene.depth, maxZ - minZ, 0.01f);
}

TEST(PlantLayoutTest, HeightsAreNormalisedPerSensorWithinUnitRange) {
    PlantLayout layout;
    auto scene = layout.layout(sample());
    float low = 0, high = 0;
    for (const auto& d : scene.devices) {
        EXPECT_GT(d.height, 0.0f);
        EXPECT_LE(d.height, 1.0f);
        if (d.device_id == "pump-001")
            low = d.height;
        if (d.device_id == "pump-002")
            high = d.height;
    }
    EXPECT_LT(low, high);
    // A sensor with a single sample sits mid-height rather than at an extreme.
    for (const auto& d : scene.devices)
        if (d.sensor == "pressure")
            EXPECT_NEAR(d.height, 0.575f, 0.01f);
}

TEST(PlantLayoutTest, PositionsAreStableAcrossValueUpdates) {
    PlantLayout layout;
    auto before = layout.layout(sample());
    auto changed = sample();
    changed[0].value = 999.0;
    auto after = layout.layout(changed);
    for (int i = 0; i < before.devices.size(); ++i) {
        EXPECT_FLOAT_EQ(before.devices[i].x, after.devices[i].x);
        EXPECT_FLOAT_EQ(before.devices[i].z, after.devices[i].z);
    }
}

TEST(PlantLayoutTest, NearestFindsTheDeviceUnderAPoint) {
    PlantLayout layout;
    auto scene = layout.layout(sample());
    const auto& target = scene.devices[2];
    EXPECT_EQ(PlantLayout::nearest(scene, target.x + 0.2f, target.z - 0.2f, 1.0f), 2);
    EXPECT_EQ(PlantLayout::nearest(scene, 1000.0f, 1000.0f, 1.0f), -1);
}

TEST(PlantLayoutTest, EmptyInputGivesEmptyScene) {
    PlantLayout layout;
    auto scene = layout.layout({});
    EXPECT_TRUE(scene.devices.isEmpty());
    EXPECT_TRUE(scene.zones.isEmpty());
}

// —— QObject wrapper fed from DeviceModel ——

class PlantSceneModelTest : public ::testing::Test {
  protected:
    static void SetUpTestSuite() {
        static int argc = 0;
        static char* argv[] = {nullptr};
        static QCoreApplication app(argc, argv);
    }
};

TEST_F(PlantSceneModelTest, RebuildsFromTheDeviceModelOnATimer) {
    ui::DeviceModel devices;
    devices.updateDeviceMeta("pump-001", "Plant A", "Floor 1");
    devices.updateDevice("pump-001", "temperature", 70.0, "C", "t", false);

    ui::PlantSceneModel scene;
    scene.setRefreshIntervalMs(10);
    QSignalSpy spy(&scene, &ui::PlantSceneModel::sceneChanged);
    scene.setSourceDevices(&devices);
    EXPECT_EQ(scene.deviceCount(), 1);

    devices.updateDevice("pump-002", "temperature", 90.0, "C", "t", true);
    EXPECT_TRUE(QTest::qWaitFor([&]() { return scene.deviceCount() == 2; }, 2000));
    EXPECT_EQ(scene.criticalCount(), 1);
    EXPECT_GE(spy.count(), 2);
}

TEST_F(PlantSceneModelTest, ExposesDevicesAndZonesToQml) {
    ui::DeviceModel devices;
    devices.updateDeviceMeta("pump-001", "Plant A", "Floor 1");
    devices.updateDevice("pump-001", "temperature", 70.0, "C", "t", true);
    ui::PlantSceneModel scene;
    scene.setSourceDevices(&devices);

    const auto list = scene.devices();
    ASSERT_EQ(list.size(), 1);
    const auto row = list[0].toMap();
    EXPECT_EQ(row.value("deviceId").toString(), "pump-001");
    EXPECT_EQ(row.value("key").toString(), "pump-001|temperature");
    EXPECT_TRUE(row.value("critical").toBool());
    EXPECT_TRUE(row.contains("x"));
    EXPECT_EQ(scene.zones().size(), 1);
    EXPECT_EQ(scene.indexOfKey("pump-001|temperature"), 0);
    EXPECT_EQ(scene.indexOfKey("nope"), -1);
}
