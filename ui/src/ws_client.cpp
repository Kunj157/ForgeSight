#include "ui/ws_client.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QUrlQuery>
#include <algorithm>
#include <cmath>

namespace ui {

WsClient::WsClient(QObject* parent) : QObject(parent) {
    connect(&socket_, &QWebSocket::connected, this, &WsClient::on_connected);
    connect(&socket_, &QWebSocket::disconnected, this, &WsClient::on_disconnected);
    connect(&socket_, &QWebSocket::textMessageReceived, this, &WsClient::on_text_message_received);
    connect(&socket_, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), this,
            &WsClient::on_error);
#ifndef QT_NO_SSL
    connect(&socket_, QOverload<const QList<QSslError>&>::of(&QWebSocket::sslErrors), this,
            &WsClient::on_ssl_errors);
#endif

    reconnect_timer_.setSingleShot(true);
    connect(&reconnect_timer_, &QTimer::timeout, this, &WsClient::try_reconnect);
}

bool WsClient::is_connected() const {
    return connected_;
}

QUrl WsClient::url() const {
    return url_;
}

void WsClient::set_url(const QUrl& url) {
    if (url_ == url)
        return;
    url_ = url;
    Q_EMIT urlChanged();
}

bool WsClient::auto_reconnect() const {
    return auto_reconnect_;
}

void WsClient::set_auto_reconnect(bool enabled) {
    if (auto_reconnect_ == enabled)
        return;
    auto_reconnect_ = enabled;
    if (!enabled)
        reconnect_timer_.stop();
    Q_EMIT autoReconnectChanged();
}

QString WsClient::api_key() const {
    return api_key_;
}

void WsClient::set_api_key(const QString& key) {
    if (api_key_ == key)
        return;
    api_key_ = key;
    Q_EMIT apiKeyChanged();
}

bool WsClient::allow_insecure_tls() const {
    return allow_insecure_tls_;
}

void WsClient::set_allow_insecure_tls(bool allow) {
    if (allow_insecure_tls_ == allow)
        return;
    allow_insecure_tls_ = allow;
    Q_EMIT allowInsecureTlsChanged();
}

QUrl WsClient::effective_url() const {
    if (api_key_.isEmpty())
        return url_;
    QUrl u = url_;
    QUrlQuery query(u.query());
    query.addQueryItem(QStringLiteral("api_key"), api_key_);
    u.setQuery(query);
    return u;
}

int WsClient::reconnect_delay_ms(int attempt) {
    const int capped = std::max(0, std::min(attempt, 15));
    const int delay = static_cast<int>(1000 * std::pow(2.0, capped));
    return std::min(delay, 30000);
}

void WsClient::connectToServer() {
    if (url_.isEmpty()) {
        Q_EMIT connectionError(QStringLiteral("No URL configured"));
        return;
    }
    manual_disconnect_ = false;
    reconnect_timer_.stop();
    if (socket_.state() == QAbstractSocket::ConnectedState ||
        socket_.state() == QAbstractSocket::ConnectingState) {
        return;
    }
    socket_.open(effective_url());
}

void WsClient::disconnectFromServer() {
    manual_disconnect_ = true;
    reconnect_timer_.stop();
    reconnect_attempt_ = 0;
    socket_.close();
}

void WsClient::try_reconnect() {
    if (!auto_reconnect_ || manual_disconnect_ || url_.isEmpty())
        return;
    if (socket_.state() == QAbstractSocket::ConnectedState ||
        socket_.state() == QAbstractSocket::ConnectingState) {
        return;
    }
    socket_.open(effective_url());
}

void WsClient::on_connected() {
    reconnect_attempt_ = 0;
    reconnect_timer_.stop();
    if (!connected_) {
        connected_ = true;
        Q_EMIT connectedChanged();
    }
}

void WsClient::on_disconnected() {
    if (connected_) {
        connected_ = false;
        Q_EMIT connectedChanged();
        Q_EMIT disconnected();
    }

    if (auto_reconnect_ && !manual_disconnect_ && !url_.isEmpty()) {
        const int delay = reconnect_delay_ms(reconnect_attempt_++);
        reconnect_timer_.start(delay);
    }
}

void WsClient::on_error(QAbstractSocket::SocketError) {
    Q_EMIT connectionError(socket_.errorString());
}

void WsClient::on_text_message_received(const QString& message) {
    auto doc = QJsonDocument::fromJson(message.toUtf8());
    if (!doc.isObject())
        return;
    auto obj = doc.object();

    if (obj.contains(QStringLiteral("device_id")) && obj.contains(QStringLiteral("sensor")) &&
        !obj.contains(QStringLiteral("severity"))) {
        Q_EMIT readingReceived(message);
    }
    if (obj.contains(QStringLiteral("severity"))) {
        Q_EMIT alarmReceived(message);
    }
}

void WsClient::on_ssl_errors(const QList<QSslError>& errors) {
#ifndef QT_NO_SSL
    if (allow_insecure_tls_) {
        socket_.ignoreSslErrors();
        return;
    }
    QStringList messages;
    for (const auto& e : errors) {
        messages << e.errorString();
    }
    qWarning("WsClient: TLS validation failed (%s); refusing to connect. Set "
            "allowInsecureTls/FORGESIGHT_ALLOW_INSECURE_TLS only for trusted self-signed "
            "deployments.",
            qUtf8Printable(messages.join(QStringLiteral("; "))));
#else
    Q_UNUSED(errors);
#endif
}

} // namespace ui
