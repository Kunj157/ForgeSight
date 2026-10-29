#pragma once

#include <QColor>
#include <QPointer>
#include <QtQuick3D/QQuick3DInstancing>

namespace ui {

class PlantSceneModel;

/// GPU-instanced geometry for the 3D plant view: one instance per device,
/// generated from a PlantSceneModel. Modes:
///   "pillars"  - a box per device, height from the live value, colour from
///                status (normal gradient / critical / selected)
///   "beacons"  - a ground ring under each critical device whose radius and
///                alpha follow `pulse` (animated from QML)
///   "pads"     - a flat footprint tile under every device
/// One draw call per mode regardless of device count, so 600 live devices
/// stay cheap; the buffer is rebuilt only when the scene, pulse or style
/// changes.
class PlantInstancing : public QQuick3DInstancing {
    Q_OBJECT
    Q_PROPERTY(QObject* scene READ scene WRITE setScene NOTIFY sceneChanged)
    Q_PROPERTY(QString mode READ mode WRITE setMode NOTIFY modeChanged)
    Q_PROPERTY(float pulse READ pulse WRITE setPulse NOTIFY pulseChanged)
    Q_PROPERTY(float rise READ rise WRITE setRise NOTIFY riseChanged)
    Q_PROPERTY(float heightScale MEMBER height_scale_ NOTIFY styleChanged)
    Q_PROPERTY(QColor lowColor MEMBER low_ NOTIFY styleChanged)
    Q_PROPERTY(QColor highColor MEMBER high_ NOTIFY styleChanged)
    Q_PROPERTY(QColor criticalColor MEMBER critical_ NOTIFY styleChanged)
    Q_PROPERTY(QColor selectedColor MEMBER selected_color_ NOTIFY styleChanged)
    Q_PROPERTY(QString selectedKey MEMBER selected_key_ NOTIFY styleChanged)

  public:
    explicit PlantInstancing(QQuick3DObject* parent = nullptr);

    QObject* scene() const;
    void setScene(QObject* scene);
    QString mode() const { return mode_; }
    void setMode(const QString& mode);
    float pulse() const { return pulse_; }
    void setPulse(float pulse);
    float rise() const { return rise_; }
    void setRise(float rise);

  Q_SIGNALS:
    void sceneChanged();
    void modeChanged();
    void pulseChanged();
    void riseChanged();
    void styleChanged();

  protected:
    QByteArray getInstanceBuffer(int* instanceCount) override;

  private:
    QPointer<PlantSceneModel> scene_;
    QString mode_ = QStringLiteral("pillars");
    float pulse_ = 0.0f;
    float rise_ = 1.0f;
    float height_scale_ = 7.0f;
    QColor low_{QStringLiteral("#2b3a58")};
    QColor high_{QStringLiteral("#5b8cff")};
    QColor critical_{QStringLiteral("#ff5d55")};
    QColor selected_color_{QStringLiteral("#ffffff")};
    QString selected_key_;
};

} // namespace ui
