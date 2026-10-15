#include "ui/device_filter_model.h"

#include "ui/device_model.h"

namespace ui {

DeviceFilterModel::DeviceFilterModel(QObject* parent) : QSortFilterProxyModel(parent) {
    setDynamicSortFilter(true);

    // The critical count is refreshed once per event-loop pass rather than
    // on every row signal, so a burst of live readings costs one recount.
    recount_timer_.setSingleShot(true);
    recount_timer_.setInterval(0);
    connect(&recount_timer_, &QTimer::timeout, this, &DeviceFilterModel::recount);

    // Row-count changes notify immediately (QML bindings on `count`).
    auto structural = [this]() {
        if (rowCount() != last_count_) {
            last_count_ = rowCount();
            Q_EMIT countChanged();
        }
        schedule_recount();
    };
    for (auto signal : {&QAbstractItemModel::rowsInserted, &QAbstractItemModel::rowsRemoved}) {
        connect(this, signal, this, structural);
    }
    connect(this, &QAbstractItemModel::modelReset, this, structural);
    connect(this, &QAbstractItemModel::layoutChanged, this, structural);
    connect(this, &QAbstractItemModel::dataChanged, this, [this]() { schedule_recount(); });
}

QObject* DeviceFilterModel::source_devices() const {
    return sourceModel();
}

void DeviceFilterModel::setSourceDevices(QObject* devices) {
    auto* model = qobject_cast<QAbstractItemModel*>(devices);
    if (model == sourceModel())
        return;
    setSourceModel(model);
    schedule_recount();
    Q_EMIT sourceDevicesChanged();
}

void DeviceFilterModel::setPlant(const QString& plant) {
    if (plant == plant_)
        return;
    plant_ = plant;
    invalidateFilter();
    Q_EMIT plantChanged();
}

void DeviceFilterModel::setFloor(const QString& floor) {
    if (floor == floor_)
        return;
    floor_ = floor;
    invalidateFilter();
    Q_EMIT floorChanged();
}

void DeviceFilterModel::setSearch(const QString& search) {
    if (search == search_)
        return;
    search_ = search;
    invalidateFilter();
    Q_EMIT searchChanged();
}

bool DeviceFilterModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const {
    const QModelIndex idx = sourceModel()->index(source_row, 0, source_parent);
    if (!plant_.isEmpty() && idx.data(DeviceModel::PlantRole).toString() != plant_)
        return false;
    if (!floor_.isEmpty() && idx.data(DeviceModel::FloorRole).toString() != floor_)
        return false;
    if (search_.isEmpty())
        return true;
    return idx.data(DeviceModel::DeviceIdRole).toString().contains(search_, Qt::CaseInsensitive) ||
           idx.data(DeviceModel::SensorRole).toString().contains(search_, Qt::CaseInsensitive);
}

int DeviceFilterModel::criticalCount() const {
    if (critical_dirty_) {
        int n = 0;
        for (int row = 0; row < rowCount(); ++row) {
            if (index(row, 0).data(DeviceModel::StatusRole).toString() == QLatin1String("critical"))
                ++n;
        }
        critical_ = n;
        critical_dirty_ = false;
    }
    return critical_;
}

void DeviceFilterModel::schedule_recount() {
    critical_dirty_ = true;
    if (!recount_timer_.isActive())
        recount_timer_.start();
}

void DeviceFilterModel::recount() {
    const int before_critical = critical_;
    const int now_critical = criticalCount();
    if (rowCount() != last_count_) {
        last_count_ = rowCount();
        Q_EMIT countChanged();
    }
    if (now_critical != before_critical)
        Q_EMIT criticalCountChanged();
}

} // namespace ui
