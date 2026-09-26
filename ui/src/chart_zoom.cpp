#include "ui/chart_zoom.h"

#include <algorithm>

namespace ui {

QVariantMap ChartZoom::range(double min, double max) {
    QVariantMap m;
    m.insert("min", min);
    m.insert("max", max);
    return m;
}

QVariantMap ChartZoom::zoom(double min, double max, double focalFraction, double scale) const {
    const double span = max - min;
    if (span <= 0.0 || scale <= 0.0)
        return range(min, max);

    const double focal = std::clamp(focalFraction, 0.0, 1.0);
    const double focalValue = min + focal * span;
    const double newSpan = span * scale;
    return range(focalValue - focal * newSpan, focalValue + (1.0 - focal) * newSpan);
}

QVariantMap ChartZoom::pan(double min, double max, double deltaFraction) const {
    const double span = max - min;
    if (span <= 0.0)
        return range(min, max);

    const double shift = deltaFraction * span;
    return range(min + shift, max + shift);
}

} // namespace ui
