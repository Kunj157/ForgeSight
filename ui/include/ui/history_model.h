#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include <QVector>

QT_BEGIN_NAMESPACE
class QQuickWindow;
QT_END_NAMESPACE

namespace ui {

struct HistoryPoint {
    QString deviceId;
    QString sensor;
    double value = 0.0;
    QString unit;
    QDateTime timestamp;
    bool anomaly = false;
};

class HistoryModel : public QAbstractListModel {
    Q_OBJECT
  public:
    enum Roles {
        DeviceIdRole = Qt::UserRole + 1,
        SensorRole,
        ValueRole,
        UnitRole,
        TimestampRole,
        AnomalyRole,
    };

    explicit HistoryModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void add_point(const QString& deviceId, const QString& sensor, double value,
                               const QString& unit, const QDateTime& timestamp,
                               bool anomaly = false);
    Q_INVOKABLE void clear();
    Q_INVOKABLE int point_count() const { return points_.size(); }
    Q_INVOKABLE bool export_csv(const QString& path);
    Q_INVOKABLE bool export_pdf(const QString& path, QQuickWindow* window);

    const QVector<HistoryPoint>& points() const { return points_; }

  private:
    QVector<HistoryPoint> points_;
};

} // namespace ui
