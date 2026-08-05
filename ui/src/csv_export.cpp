#include "ui/csv_export.h"

#include <QFile>
#include <QTextStream>

namespace ui {

QString escape_csv(const QString& field) {
    if (field.contains(',') || field.contains('"') || field.contains('\n')) {
        QString escaped = field;
        escaped.replace("\"", "\"\"");
        return "\"" + escaped + "\"";
    }
    return field;
}

QString CsvExport::to_string(const QVector<HistoryPoint>& points) {
    QString out;
    QTextStream s(&out);
    s << "DeviceId,Sensor,Value,Unit,Timestamp,Anomaly\n";
    for (const auto& p : points) {
        s << escape_csv(p.deviceId) << ',' << escape_csv(p.sensor) << ',' << p.value << ','
          << escape_csv(p.unit) << ',' << p.timestamp.toString(Qt::ISODate) << ','
          << (p.anomaly ? "true" : "false") << '\n';
    }
    return out;
}

bool CsvExport::to_file(const QString& filePath, const QVector<HistoryPoint>& points) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    file.write(to_string(points).toUtf8());
    file.close();
    return true;
}

} // namespace ui
