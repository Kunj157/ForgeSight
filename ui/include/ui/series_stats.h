#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

namespace ui {

// QML-exposed helper that summarises a short window of sensor values for a
// device card: min/max range (context) and the latest change (trend chip).
// Pure logic, unit-tested independently of any UI.
class SeriesStats : public QObject {
    Q_OBJECT

  public:
    explicit SeriesStats(QObject* parent = nullptr);

    // Given a series of numeric values (non-numeric entries are ignored),
    // returns a map with:
    //   valid      (bool)   - false when the series has no numeric values
    //   count      (int)    - number of numeric values considered
    //   min, max   (double) - range over the whole window
    //   first,last (double) - first/last numeric value
    //   delta      (double) - last minus the previous numeric value (0 if <2)
    //   direction  (string) - "up" / "down" / "flat" based on delta
    Q_INVOKABLE QVariantMap compute(const QVariantList& values) const;
};

} // namespace ui
