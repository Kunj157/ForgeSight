#pragma once

#include <QDateTime>
#include <QObject>
#include <QString>

namespace ui {

// Small QML-exposed helper that turns the backend's raw timestamp strings
// into human-friendly text for the dashboard. Kept as a plain QObject with
// pure static/const methods so the formatting logic is unit-testable without
// instantiating any UI.
class TimeFormat : public QObject {
    Q_OBJECT

  public:
    explicit TimeFormat(QObject* parent = nullptr);

    // Compact local wall-clock time, "HH:mm:ss". Empty if unparseable.
    Q_INVOKABLE QString clockTime(const QString& iso) const;

    // Human-friendly age relative to now: "just now", "12s ago", "5m ago",
    // "3h ago", "2d ago". Empty if unparseable.
    Q_INVOKABLE QString relative(const QString& iso) const;

    // Same as relative() but against an explicit reference time (testable,
    // deterministic).
    QString relativeFrom(const QString& iso, const QDateTime& now) const;

    // Parses both the "T"-separated ISO 8601 form and the space-separated
    // Postgres `timestamptz::text` form, tolerating fractional seconds and a
    // short ("+02") or full ("+02:00") timezone offset. Returns an invalid
    // QDateTime on failure.
    static QDateTime parse(const QString& iso);
};

} // namespace ui
