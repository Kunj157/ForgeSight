#include "ui/series_stats.h"

#include <cmath>
#include <vector>

namespace ui {

SeriesStats::SeriesStats(QObject* parent) : QObject(parent) {}

QVariantMap SeriesStats::compute(const QVariantList& values) const {
    std::vector<double> nums;
    nums.reserve(static_cast<std::size_t>(values.size()));
    for (const QVariant& v : values) {
        bool ok = false;
        const double d = v.toDouble(&ok);
        if (ok)
            nums.push_back(d);
    }

    QVariantMap out;
    if (nums.empty()) {
        out.insert("valid", false);
        return out;
    }

    double minV = nums.front();
    double maxV = nums.front();
    for (double d : nums) {
        if (d < minV)
            minV = d;
        if (d > maxV)
            maxV = d;
    }

    const double first = nums.front();
    const double last = nums.back();
    const double delta = nums.size() >= 2 ? last - nums[nums.size() - 2] : 0.0;

    QString direction = "flat";
    if (std::abs(delta) > 1e-9)
        direction = delta > 0 ? "up" : "down";

    out.insert("valid", true);
    out.insert("count", static_cast<int>(nums.size()));
    out.insert("min", minV);
    out.insert("max", maxV);
    out.insert("first", first);
    out.insert("last", last);
    out.insert("delta", delta);
    out.insert("direction", direction);
    return out;
}

} // namespace ui
