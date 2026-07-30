#pragma once

#include <QObject>
#include <QTimer>
#include <QUrl>
#include <QWebSocket>

namespace ui {

class WsClient : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool connected READ is_connected NOTIFY connectedChanged)
    Q_PROPERTY(QUrl url READ url WRITE set_url NOTIFY urlChanged)
    Q_PROPERTY(
        bool autoReconnect READ auto_reconnect WRITE set_auto_reconnect NOTIFY autoReconnectChanged)

  public:
    explicit WsClient(QObject* parent = nullptr);

    bool is_connected() const;
    QUrl url() const;
    void set_url(const QUrl& url);

    bool auto_reconnect() const;
    void set_auto_reconnect(bool enabled);

    /// Exponential backoff: 1s, 2s, 4s… capped at 30s.
    static int reconnect_delay_ms(int attempt);

    Q_INVOKABLE void connectToServer();
    Q_INVOKABLE void disconnectFromServer();

  Q_SIGNALS:
    void connectedChanged();
    void urlChanged();
    void autoReconnectChanged();
    void readingReceived(const QString& json);
    void alarmReceived(const QString& json);
    void connectionError(const QString& error);
    void disconnected();

  private Q_SLOTS:
    void on_connected();
    void on_disconnected();
    void on_text_message_received(const QString& message);
    void on_ssl_errors(const QList<QSslError>& errors);
    void on_error(QAbstractSocket::SocketError error);
    void try_reconnect();

  private:
    QWebSocket socket_;
    QUrl url_;
    QTimer reconnect_timer_;
    bool connected_ = false;
    bool auto_reconnect_ = true;
    bool manual_disconnect_ = false;
    int reconnect_attempt_ = 0;
};

} // namespace ui
