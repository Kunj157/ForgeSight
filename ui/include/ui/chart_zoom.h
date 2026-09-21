#pragma once

#include <QObject>
#include <QVariantMap>

namespace ui {

/// Stateless helper exposing the pan/zoom range math to QML, so HistoryPanel's
/// chart interaction is thin and the arithmetic (focal-point-preserving zoom,
/// fractional pan) is unit-tested. All ranges are plain doubles; QML passes
/// DateTimeAxis bounds as msecs (Date.getTime()) and rebuilds Dates from the
/// result. Returns a {"min","max"} map.
class ChartZoom : public QObject {
    Q_OBJECT

  public:
    explicit ChartZoom(QObject* parent = nullptr) : QObject(parent) {}

    /// Scales [min,max] by `scale` about `focalFraction` (0=left .. 1=right),
    /// keeping the value under the focal point stationary. scale<1 zooms in,
    /// scale>1 zooms out. focalFraction is clamped to [0,1]. Returns the range
    /// unchanged when the span is non-positive or scale is non-positive.
    Q_INVOKABLE QVariantMap zoom(double min, double max, double focalFraction, double scale) const;

    /// Shifts [min,max] by `deltaFraction` of the current span (positive moves
    /// the window toward larger values). Returns unchanged for a non-positive
    /// span.
    Q_INVOKABLE QVariantMap pan(double min, double max, double deltaFraction) const;

  private:
    static QVariantMap range(double min, double max);
};

} // namespace ui
