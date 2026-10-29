#include "ui/plant_scene_model.h"

#include <QVariantMap>

#include "ui/device_model.h"

namespace ui {

PlantSceneModel::PlantSceneModel(QObject* parent) : QObject(parent) {
    timer_.setSingleShot(true);
    timer_.setInterval(250);
    connect(&timer_, &QTimer::timeout, this, &PlantSceneModel::rebuild);
}

QObject* PlantSceneModel::source_devices() const {
    return source_;
}

void PlantSceneModel::setSourceDevices(QObject* devices) {
    auto* model = qobject_cast<DeviceModel*>(devices);
    if (model == source_)
        return;
    if (source_)
        disconnect(source_, nullptr, this, nullptr);
    source_ = model;
    if (source_) {
        connect(source_, &QAbstractItemModel::dataChanged, this, &PlantSceneModel::schedule);
        connect(source_, &QAbstractItemModel::rowsInserted, this, &PlantSceneModel::schedule);
        connect(source_, &QAbstractItemModel::rowsRemoved, this, &PlantSceneModel::schedule);
        connect(source_, &QAbstractItemModel::modelReset, this, &PlantSceneModel::schedule);
    }
    rebuild();
    Q_EMIT sourceDevicesChanged();
}

void PlantSceneModel::schedule() {
    if (!timer_.isActive())
        timer_.start();
}

void PlantSceneModel::rebuild() {
    QVector<PlantDevice> input;
    if (source_) {
        const int rows = source_->rowCount();
        input.reserve(rows);
        for (int r = 0; r < rows; ++r) {
            const QModelIndex idx = source_->index(r);
            input.push_back(
                {idx.data(DeviceModel::DeviceIdRole).toString(),
                 idx.data(DeviceModel::SensorRole).toString(),
                 idx.data(DeviceModel::PlantRole).toString(),
                 idx.data(DeviceModel::FloorRole).toString(),
                 idx.data(DeviceModel::ValueRole).toDouble(),
                 idx.data(DeviceModel::StatusRole).toString() == QLatin1String("critical")});
        }
    }
    scene_ = layout_.layout(input);
    Q_EMIT sceneChanged();
}

int PlantSceneModel::criticalCount() const {
    int n = 0;
    for (const auto& d : scene_.devices)
        n += d.critical ? 1 : 0;
    return n;
}

QVariantList PlantSceneModel::devices() const {
    QVariantList out;
    out.reserve(scene_.devices.size());
    for (const auto& d : scene_.devices) {
        out.push_back(QVariantMap{
            {"key", d.device_id + QLatin1Char('|') + d.sensor},
            {"deviceId", d.device_id},
            {"sensor", d.sensor},
            {"plant", d.plant},
            {"floor", d.floor},
            {"value", d.value},
            {"x", d.x},
            {"z", d.z},
            {"height", d.height},
            {"critical", d.critical},
        });
    }
    return out;
}

QVariantList PlantSceneModel::zones() const {
    QVariantList out;
    for (const auto& z : scene_.zones) {
        out.push_back(QVariantMap{
            {"plant", z.plant},
            {"floor", z.floor},
            {"x", z.x},
            {"z", z.z},
            {"width", z.width},
            {"depth", z.depth},
            {"count", z.count},
            {"critical", z.critical},
        });
    }
    return out;
}

int PlantSceneModel::pick(qreal x, qreal z, qreal maxDist) const {
    return PlantLayout::nearest(scene_, float(x), float(z), float(maxDist));
}

int PlantSceneModel::indexOfKey(const QString& key) const {
    for (int i = 0; i < scene_.devices.size(); ++i) {
        const auto& d = scene_.devices[i];
        if (d.device_id + QLatin1Char('|') + d.sensor == key)
            return i;
    }
    return -1;
}

} // namespace ui
