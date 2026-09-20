#include "ui/time_format.h"

#include <QRegularExpression>

namespace ui {

TimeFormat::TimeFormat(QObject* parent) : QObject(parent) {}

QDateTime TimeFormat::parse(const QString& iso) {
    QString s = iso.trimmed();
    if (s.isEmpty())
        return {};

    // Postgres `timestamptz::text` uses a space between date and time; ISO 8601
    // uses 'T'. Normalise to 'T' so a single parser handles both.
    if (s.size() > 10 && s.at(10) == QLatin1Char(' '))
        s[10] = QLatin1Char('T');

    // Normalise a short timezone offset ("+02") to the full form ("+02:00")
    // that Qt's ISODate parser expects.
    static const QRegularExpression shortOffset(QStringLiteral("[+-]\\d{2}$"));
    if (shortOffset.match(s).hasMatch())
        s += QStringLiteral(":00");

    QDateTime dt = QDateTime::fromString(s, Qt::ISODateWithMs);
    if (!dt.isValid())
        dt = QDateTime::fromString(s, Qt::ISODate);
    return dt;
}

QString TimeFormat::clockTime(const QString& iso) const {
    const QDateTime dt = parse(iso);
    if (!dt.isValid())
        return {};
    return dt.toLocalTime().toString(QStringLiteral("HH:mm:ss"));
}

QString TimeFormat::relativeFrom(const QString& iso, const QDateTime& now) const {
    const QDateTime dt = parse(iso);
    if (!dt.isValid())
        return {};

    qint64 secs = dt.secsTo(now);
    if (secs < 0)
        secs = 0; // tolerate minor clock skew (timestamp slightly in the future)

    if (secs < 2)
        return QStringLiteral("just now");
    if (secs < 60)
        return QStringLiteral("%1s ago").arg(secs);

    const qint64 mins = secs / 60;
    if (mins < 60)
        return QStringLiteral("%1m ago").arg(mins);

    const qint64 hours = mins / 60;
    if (hours < 24)
        return QStringLiteral("%1h ago").arg(hours);

    const qint64 days = hours / 24;
    return QStringLiteral("%1d ago").arg(days);
}

QString TimeFormat::relative(const QString& iso) const {
    return relativeFrom(iso, QDateTime::currentDateTime());
}

} // namespace ui
