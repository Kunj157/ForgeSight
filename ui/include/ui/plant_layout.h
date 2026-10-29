#pragma once

#include <QHash>
#include <QPair>
#include <QString>
#include <QVector>

namespace ui {

/// One sensor reading as the 3D plant view sees it.
struct PlantDevice {
    QString device_id;
    QString sensor;
    QString plant;
    QString floor;
    double value = 0.0;
    bool critical = false;
};

/// A device placed on the factory floor. x/z are world units on the ground
/// plane; height is the reading normalised per sensor to (0, 1].
struct PlacedDevice {
    QString device_id;
    QString sensor;
    QString plant;
    QString floor;
    double value = 0.0;
    float x = 0.0f;
    float z = 0.0f;
    float height = 0.5f;
    bool critical = false;
    int zone = 0;
};

/// A plant/floor production area: a rectangle on the ground plane.
struct PlantZone {
    QString plant;
    QString floor;
    float x = 0.0f;
    float z = 0.0f;
    float width = 0.0f;
    float depth = 0.0f;
    int count = 0;
    int critical = 0;
};

struct PlantScene {
    QVector<PlacedDevice> devices;
    QVector<PlantZone> zones;
    float width = 0.0f;
    float depth = 0.0f;
};

/// Pure layout for the 3D plant overview: plants side by side along X,
/// their floors as zones stacked along Z, devices on a grid inside each
/// zone, the whole scene centred on the origin. Positions depend only on
/// the set of devices (sorted), so live value updates never move a pillar.
/// Heights use a running per-sensor min/max, so the layout object is kept
/// across frames.
class PlantLayout {
  public:
    float spacing = 2.4f;
    float zone_gap = 3.0f;
    float plant_gap = 6.0f;
    int max_columns = 12;

    PlantScene layout(const QVector<PlantDevice>& devices);

    /// Index of the device closest to (x, z) within max_dist, or -1.
    static int nearest(const PlantScene& scene, float x, float z, float max_dist);

  private:
    float normalise(const QString& sensor, double value);

    QHash<QString, QPair<double, double>> ranges_; // sensor -> (min, max)
};

} // namespace ui
