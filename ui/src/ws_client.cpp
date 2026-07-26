#include "ui/ws_client.h"

#include <QJsonDocument>
#include <QJsonObject>

namespace ui {

WsClient::WsClient(QObject* parent)
    : QObject(parent) {
    connect(&socket_, &QWebSocket::connected, this, &WsClient::on_connected);
    connect(&socket_, &QWebSocket::disconnected, this, &WsClient::on_disconnected);
    connect(&socket_, &QWebSocket::textMessageReceived, this, &WsClient::on_text_message_received);
#ifndef QT_NO_SSL
    connect(&socket_, QOverload<const QList<QSslError>&>::of(&QWebSocket::sslErrors),
            this, &WsClient::on_ssl_errors);
#endif
}

bool WsClient::is_connected() const { return connected_; }

QUrl WsClient::url() const { return url_; }

void WsClient::set_url(const QUrl& url) {
    if (url_ == url) return;
    url_ = url;
    Q_EMIT urlChanged();
}

void WsClient::connectToServer() {
    if (url_.isEmpty()) {
        Q_EMIT connectionError("No URL configured");
        return;
    }
    socket_.open(url_);
}

void WsClient::disconnectFromServer() {
    socket_.close();
}

void WsClient::on_connected() {
    connected_ = true;
    Q_EMIT connectedChanged();
}

void WsClient::on_disconnected() {
    connected_ = false;
    Q_EMIT connectedChanged();
    Q_EMIT disconnected();
}

void WsClient::on_text_message_received(const QString& message) {
    auto doc = QJsonDocument::fromJson(message.toUtf8());
    if (!doc.isObject()) return;
    auto obj = doc.object();

    if (obj.contains("device_id") && obj.contains("sensor")) {
        Q_EMIT readingReceived(message);
    }
    if (obj.contains("severity")) {
        Q_EMIT alarmReceived(message);
    }
}

void WsClient::on_ssl_errors(const QList<QSslError>& errors) {
    Q_UNUSED(errors);
#ifndef QT_NO_SSL
    socket_.ignoreSslErrors();
#endif
}

}  // namespace ui
