#pragma once

#include <QObject>
#include <QUrl>
#include <QWebSocket>

namespace ui {

class WsClient : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool connected READ is_connected NOTIFY connectedChanged)
    Q_PROPERTY(QUrl url READ url WRITE set_url NOTIFY urlChanged)

public:
    explicit WsClient(QObject* parent = nullptr);

    bool is_connected() const;
    QUrl url() const;
    void set_url(const QUrl& url);

    Q_INVOKABLE void connectToServer();
    Q_INVOKABLE void disconnectFromServer();

Q_SIGNALS:
    void connectedChanged();
    void urlChanged();
    void readingReceived(const QString& json);
    void alarmReceived(const QString& json);
    void connectionError(const QString& error);
    void disconnected();

private Q_SLOTS:
    void on_connected();
    void on_disconnected();
    void on_text_message_received(const QString& message);
    void on_ssl_errors(const QList<QSslError>& errors);

private:
    QWebSocket socket_;
    QUrl url_;
    bool connected_ = false;
};

}  // namespace ui
