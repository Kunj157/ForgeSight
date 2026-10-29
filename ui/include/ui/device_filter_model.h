#pragma once

#include <QSortFilterProxyModel>
#include <QString>
#include <QTimer>

namespace ui {

class DeviceModel;

/// Live, filtered view of a DeviceModel for one section of the device tree
/// (a plant, a floor of a plant, or everything), optionally narrowed by a
/// search string matched case-insensitively against device id and sensor.
///
/// Unlike a QVariantList snapshot from DeviceModel::devicesFor(), rows keep
/// their identity: a reading for a visible device arrives as dataChanged on
/// that one row, so QML delegates update in place instead of the whole
/// section being rebuilt.
class DeviceFilterModel : public QSortFilterProxyModel {
    Q_OBJECT
    Q_PROPERTY(QObject* sourceDevices READ source_devices WRITE setSourceDevices NOTIFY
                   sourceDevicesChanged)
    Q_PROPERTY(QString plant READ plant WRITE setPlant NOTIFY plantChanged)
    Q_PROPERTY(QString floor READ floor WRITE setFloor NOTIFY floorChanged)
    Q_PROPERTY(QString search READ search WRITE setSearch NOTIFY searchChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int criticalCount READ criticalCount NOTIFY criticalCountChanged)

  public:
    explicit DeviceFilterModel(QObject* parent = nullptr);

    QObject* source_devices() const;
    void setSourceDevices(QObject* devices);

    QString plant() const { return plant_; }
    void setPlant(const QString& plant);
    QString floor() const { return floor_; }
    void setFloor(const QString& floor);
    QString search() const { return search_; }
    void setSearch(const QString& search);

    int count() const { return rowCount(); }
    int criticalCount() const;

  Q_SIGNALS:
    void sourceDevicesChanged();
    void plantChanged();
    void floorChanged();
    void searchChanged();
    void countChanged();
    void criticalCountChanged();

  protected:
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;

  private:
    void schedule_recount();
    void recount();

    QString plant_;
    QString floor_;
    QString search_;
    int last_count_ = 0;
    mutable int critical_ = 0;
    mutable bool critical_dirty_ = true;
    QTimer recount_timer_;
};

} // namespace ui
