#include "plant_instancing.h"

#include <QVector3D>
#include <QVector4D>

#include "ui/plant_scene_model.h"

namespace ui {

namespace {
// The built-in #Cube / #Cylinder meshes are 100 units across.
constexpr float kMesh = 100.0f;
constexpr float kPillarWidth = 1.25f;

QColor mix(const QColor& a, const QColor& b, float t) {
    return QColor::fromRgbF(
        a.redF() + (b.redF() - a.redF()) * t, a.greenF() + (b.greenF() - a.greenF()) * t,
        a.blueF() + (b.blueF() - a.blueF()) * t, a.alphaF() + (b.alphaF() - a.alphaF()) * t);
}
} // namespace

PlantInstancing::PlantInstancing(QQuick3DObject* parent) : QQuick3DInstancing(parent) {
    connect(this, &PlantInstancing::styleChanged, this, [this]() { markDirty(); });
}

QObject* PlantInstancing::scene() const {
    return scene_;
}

void PlantInstancing::setScene(QObject* scene) {
    auto* model = qobject_cast<PlantSceneModel*>(scene);
    if (model == scene_)
        return;
    if (scene_)
        disconnect(scene_, nullptr, this, nullptr);
    scene_ = model;
    if (scene_)
        connect(scene_, &PlantSceneModel::sceneChanged, this, [this]() { markDirty(); });
    markDirty();
    Q_EMIT sceneChanged();
}

void PlantInstancing::setMode(const QString& mode) {
    if (mode == mode_)
        return;
    mode_ = mode;
    markDirty();
    Q_EMIT modeChanged();
}

void PlantInstancing::setPulse(float pulse) {
    if (qFuzzyCompare(pulse, pulse_))
        return;
    pulse_ = pulse;
    if (mode_ == QLatin1String("beacons"))
        markDirty();
    Q_EMIT pulseChanged();
}

void PlantInstancing::setRise(float rise) {
    if (qFuzzyCompare(rise, rise_))
        return;
    rise_ = rise;
    if (mode_ == QLatin1String("pillars"))
        markDirty();
    Q_EMIT riseChanged();
}

QByteArray PlantInstancing::getInstanceBuffer(int* instanceCount) {
    QByteArray buffer;
    int count = 0;
    if (scene_) {
        const auto& devices = scene_->scene().devices;
        buffer.reserve(devices.size() * int(sizeof(InstanceTableEntry)));
        for (const auto& d : devices) {
            const QString key = d.device_id + QLatin1Char('|') + d.sensor;
            const bool selected = !selected_key_.isEmpty() && key == selected_key_;
            InstanceTableEntry entry;
            if (mode_ == QLatin1String("pillars")) {
                const float h = std::max(0.05f, d.height * height_scale_ * rise_);
                QColor c = d.critical ? critical_ : mix(low_, high_, d.height);
                if (selected)
                    c = mix(c, selected_color_, 0.55f);
                entry = calculateTableEntry(
                    QVector3D(d.x, h / 2, d.z),
                    QVector3D(kPillarWidth / kMesh, h / kMesh, kPillarWidth / kMesh), QVector3D(),
                    c);
            } else if (mode_ == QLatin1String("beacons")) {
                if (!d.critical && !selected)
                    continue;
                const float r = selected && !d.critical ? 2.0f : 1.4f + 2.6f * pulse_;
                QColor c = d.critical ? critical_ : selected_color_;
                c.setAlphaF(d.critical ? 0.85f * (1.0f - pulse_) : 0.6f);
                entry = calculateTableEntry(QVector3D(d.x, 0.16f, d.z),
                                            QVector3D(r / kMesh, 0.02f / kMesh, r / kMesh),
                                            QVector3D(), c);
            } else { // pads
                QColor c = d.critical ? critical_ : high_;
                c.setAlphaF(d.critical ? 0.35f : 0.12f);
                entry = calculateTableEntry(QVector3D(d.x, 0.14f, d.z),
                                            QVector3D(2.0f / kMesh, 0.02f / kMesh, 2.0f / kMesh),
                                            QVector3D(), c);
            }
            buffer.append(reinterpret_cast<const char*>(&entry), sizeof(entry));
            ++count;
        }
    }
    if (instanceCount)
        *instanceCount = count;
    return buffer;
}

} // namespace ui
