#pragma once

#include "ui/live_feed.h"

#include <QObject>
#include <QTimer>
#include <QUrl>
#include <QWebSocket>

namespace ui {

class WsClient : public LiveFeed {
    Q_OBJECT
    Q_PROPERTY(
        bool autoReconnect READ auto_reconnect WRITE set_auto_reconnect NOTIFY autoReconnectChanged)
    Q_PROPERTY(QString apiKey READ api_key WRITE set_api_key NOTIFY apiKeyChanged)
    Q_PROPERTY(bool allowInsecureTls READ allow_insecure_tls WRITE set_allow_insecure_tls NOTIFY
                   allowInsecureTlsChanged)

  public:
    explicit WsClient(QObject* parent = nullptr);

    bool is_connected() const override;
    QUrl url() const override;
    void set_url(const QUrl& url) override;

    bool auto_reconnect() const;
    void set_auto_reconnect(bool enabled);

    QString api_key() const;
    void set_api_key(const QString& key);

    // Gates WsClient::on_ssl_errors()'s use of QWebSocket::ignoreSslErrors().
    // Defaults to false so a release build never silently bypasses TLS
    // validation; an operator must opt in explicitly (e.g. for a self-signed
    // cert in a trusted LAN deployment).
    bool allow_insecure_tls() const;
    void set_allow_insecure_tls(bool allow);

    /// Exponential backoff: 1s, 2s, 4s… capped at 30s.
    static int reconnect_delay_ms(int attempt);

    Q_INVOKABLE void connectToServer() override;
    Q_INVOKABLE void disconnectFromServer() override;

  Q_SIGNALS:
    void apiKeyChanged();
    void autoReconnectChanged();
    void allowInsecureTlsChanged();

  private Q_SLOTS:
    void on_connected();
    void on_disconnected();
    void on_text_message_received(const QString& message);
    void on_ssl_errors(const QList<QSslError>& errors);
    void on_error(QAbstractSocket::SocketError error);
    void try_reconnect();

  private:
    QUrl effective_url() const;

    QWebSocket socket_;
    QUrl url_;
    QString api_key_;
    QTimer reconnect_timer_;
    bool connected_ = false;
    bool auto_reconnect_ = true;
    bool manual_disconnect_ = false;
    bool allow_insecure_tls_ = false;
    int reconnect_attempt_ = 0;
};

} // namespace ui
