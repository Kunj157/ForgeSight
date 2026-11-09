#pragma once

#include <QObject>
#include <QString>
#include <QUrl>

namespace ui {

/// Server-push surface: live readings and alarms arrive as JSON text, one
/// message per signal emission. WsClient implements it against the backend's
/// WebSocket; the browser demo implements it with its simulator's tick.
class LiveFeed : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool connected READ is_connected NOTIFY connectedChanged)
    Q_PROPERTY(QUrl url READ url WRITE set_url NOTIFY urlChanged)

  public:
    explicit LiveFeed(QObject* parent = nullptr) : QObject(parent) {}

    virtual bool is_connected() const = 0;
    virtual QUrl url() const = 0;
    virtual void set_url(const QUrl& url) = 0;

    Q_INVOKABLE virtual void connectToServer() = 0;
    Q_INVOKABLE virtual void disconnectFromServer() = 0;

  Q_SIGNALS:
    void connectedChanged();
    void urlChanged();
    void readingReceived(const QString& json);
    void alarmReceived(const QString& json);
    void connectionError(const QString& error);
    void disconnected();
};

} // namespace ui
