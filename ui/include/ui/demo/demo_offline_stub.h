#pragma once

#include <QObject>

namespace ui::demo {

/// Stands in for ui::OfflineCache in the browser build, which has no SQLite.
/// Exposes the same QML-facing members, with nothing persisted and no acks
/// ever pending (the demo backend acknowledges synchronously).
class OfflineCacheStub : public QObject {
    Q_OBJECT
    Q_PROPERTY(int pendingAckCount READ pending_ack_count NOTIFY pendingAckCountChanged)

  public:
    explicit OfflineCacheStub(QObject* parent = nullptr) : QObject(parent) {}

    int pending_ack_count() const { return 0; }

    Q_INVOKABLE void queueAck(qint64 /*alarm_id*/) {}

  Q_SIGNALS:
    void pendingAckCountChanged();
};

} // namespace ui::demo
