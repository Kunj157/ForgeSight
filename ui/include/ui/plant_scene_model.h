#pragma once

#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QVariantList>

#include "ui/plant_layout.h"

namespace ui {

class DeviceModel;

/// Live PlantScene for the 3D (and 2.5D fallback) plant view. Watches a
/// DeviceModel and re-lays the scene out on a short coalescing timer, so a
/// burst of readings costs one layout pass. Exposes the scene to QML as
/// plain lists and to C++ consumers (the instanced renderer) directly.
class PlantSceneModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(QObject* sourceDevices READ source_devices WRITE setSourceDevices NOTIFY
                   sourceDevicesChanged)
    Q_PROPERTY(int deviceCount READ deviceCount NOTIFY sceneChanged)
    Q_PROPERTY(int criticalCount READ criticalCount NOTIFY sceneChanged)
    Q_PROPERTY(qreal sceneWidth READ scene_width NOTIFY sceneChanged)
    Q_PROPERTY(qreal sceneDepth READ scene_depth NOTIFY sceneChanged)

  public:
    explicit PlantSceneModel(QObject* parent = nullptr);

    QObject* source_devices() const;
    void setSourceDevices(QObject* devices);
    void setRefreshIntervalMs(int ms) { timer_.setInterval(ms); }

    const PlantScene& scene() const { return scene_; }
    int deviceCount() const { return scene_.devices.size(); }
    int criticalCount() const;
    qreal scene_width() const { return scene_.width; }
    qreal scene_depth() const { return scene_.depth; }

    /// [{ key, deviceId, sensor, plant, floor, value, x, z, height, critical }]
    Q_INVOKABLE QVariantList devices() const;
    /// [{ plant, floor, x, z, width, depth, count, critical }]
    Q_INVOKABLE QVariantList zones() const;
    /// Device under a ground-plane point, or -1.
    Q_INVOKABLE int pick(qreal x, qreal z, qreal maxDist = 1.2) const;
    Q_INVOKABLE int indexOfKey(const QString& key) const;

  public Q_SLOTS:
    void rebuild();

  Q_SIGNALS:
    void sourceDevicesChanged();
    void sceneChanged();

  private:
    void schedule();

    QPointer<DeviceModel> source_;
    PlantLayout layout_;
    PlantScene scene_;
    QTimer timer_;
};

} // namespace ui
