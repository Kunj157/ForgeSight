#include "ui/history_model.h"

#include "ui/browser_download.h"
#include "ui/csv_export.h"
#include "ui/pdf_export.h"

#include <QDir>
#include <QFileInfo>
#include <QQuickWindow>
#include <QStandardPaths>

namespace ui {

HistoryModel::HistoryModel(QObject* parent) : QAbstractListModel(parent) {}

int HistoryModel::rowCount(const QModelIndex&) const {
    return points_.size();
}

QVariant HistoryModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= points_.size())
        return {};

    const auto& p = points_[index.row()];
    switch (role) {
    case DeviceIdRole:
        return p.deviceId;
    case SensorRole:
        return p.sensor;
    case ValueRole:
        return p.value;
    case UnitRole:
        return p.unit;
    case TimestampRole:
        return p.timestamp;
    case AnomalyRole:
        return p.anomaly;
    default:
        return {};
    }
}

QHash<int, QByteArray> HistoryModel::roleNames() const {
    return {
        {DeviceIdRole, "deviceId"}, {SensorRole, "sensor"},       {ValueRole, "value"},
        {UnitRole, "unit"},         {TimestampRole, "timestamp"}, {AnomalyRole, "anomaly"},
    };
}

void HistoryModel::add_point(const QString& deviceId, const QString& sensor, double value,
                             const QString& unit, const QDateTime& timestamp, bool anomaly) {
    beginInsertRows({}, points_.size(), points_.size());
    points_.push_back({deviceId, sensor, value, unit, timestamp, anomaly});
    endInsertRows();
    Q_EMIT pointCountChanged();
}

void HistoryModel::clear() {
    beginResetModel();
    points_.clear();
    endResetModel();
    Q_EMIT pointCountChanged();
}

bool HistoryModel::export_csv(const QString& path) {
#ifdef Q_OS_WASM
    // In the browser the file goes to the visitor's downloads.
    return browser_download(CsvExport::to_string(points_).toUtf8(), QFileInfo(path).fileName(),
                            QStringLiteral("text/csv"));
#else
    return CsvExport::to_file(path, points_);
#endif
}

bool HistoryModel::export_pdf(const QString& path, QQuickWindow* window) {
#ifdef Q_OS_WASM
    const QByteArray pdf = PdfExport::to_bytes(window, points_);
    return !pdf.isEmpty() &&
           browser_download(pdf, QFileInfo(path).fileName(), QStringLiteral("application/pdf"));
#else
    return PdfExport::export_chart(path, window, points_);
#endif
}

QString HistoryModel::default_export_path(const QString& filename) const {
#ifdef Q_OS_WASM
    // The browser saves downloads where the visitor chooses; only the name matters.
    return filename;
#endif
    QString dirPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (dirPath.isEmpty())
        dirPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    dirPath += QStringLiteral("/ForgeSight");

    QDir().mkpath(dirPath);
    return dirPath + QStringLiteral("/") + filename;
}

} // namespace ui
