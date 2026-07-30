#include "ui/api_client.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>

namespace ui {

ApiClient::ApiClient(QObject* parent) : QObject(parent) {}

QUrl ApiClient::base_url() const {
    return base_url_;
}

void ApiClient::set_base_url(const QUrl& url) {
    if (base_url_ == url)
        return;
    base_url_ = url;
    Q_EMIT baseUrlChanged();
}

bool ApiClient::is_busy() const {
    return busy_;
}

void ApiClient::set_busy(bool busy) {
    if (busy_ == busy)
        return;
    busy_ = busy;
    Q_EMIT busyChanged();
}

void ApiClient::bootstrap() {
    fetchLatestReadings();
    fetchAlarms();
}

void ApiClient::get_json(const QString& path, const std::function<void(const QByteArray&)>& on_ok) {
    QUrl url = base_url_;
    url.setPath(path);

    ++pending_;
    set_busy(true);

    auto* reply = nam_.get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply, on_ok]() {
        reply->deleteLater();
        const auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() != QNetworkReply::NoError || status >= 400) {
            Q_EMIT requestFailed(reply->errorString().isEmpty()
                                     ? QStringLiteral("HTTP %1").arg(status)
                                     : reply->errorString());
        } else {
            on_ok(reply->readAll());
        }

        if (--pending_ <= 0) {
            pending_ = 0;
            set_busy(false);
            Q_EMIT bootstrapFinished(true, {});
        }
    });
}

void ApiClient::post_json(const QString& path, const QByteArray& body,
                          const std::function<void(int, const QByteArray&)>& on_done) {
    QUrl url = base_url_;
    url.setPath(path);

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    ++pending_;
    set_busy(true);

    auto* reply = nam_.post(req, body);
    connect(reply, &QNetworkReply::finished, this, [this, reply, on_done]() {
        reply->deleteLater();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        on_done(status, reply->readAll());
        if (--pending_ <= 0) {
            pending_ = 0;
            set_busy(false);
        }
    });
}

void ApiClient::fetchLatestReadings() {
    get_json(QStringLiteral("/api/readings/latest"), [this](const QByteArray& body) {
        const auto doc = QJsonDocument::fromJson(body);
        if (!doc.isArray()) {
            Q_EMIT requestFailed(QStringLiteral("Invalid readings payload"));
            return;
        }
        for (const auto& v : doc.array()) {
            const auto o = v.toObject();
            Q_EMIT readingReceived(o.value(QStringLiteral("device_id")).toString(),
                                   o.value(QStringLiteral("sensor")).toString(),
                                   o.value(QStringLiteral("value")).toDouble(),
                                   o.value(QStringLiteral("unit")).toString(),
                                   o.value(QStringLiteral("timestamp")).toString(),
                                   o.value(QStringLiteral("anomaly")).toBool());
        }
    });
}

void ApiClient::fetchAlarms() {
    get_json(QStringLiteral("/api/alarms"), [this](const QByteArray& body) {
        const auto doc = QJsonDocument::fromJson(body);
        if (!doc.isArray()) {
            Q_EMIT requestFailed(QStringLiteral("Invalid alarms payload"));
            return;
        }
        for (const auto& v : doc.array()) {
            const auto o = v.toObject();
            Q_EMIT alarmReceived(static_cast<qint64>(o.value(QStringLiteral("id")).toDouble()),
                                 o.value(QStringLiteral("device_id")).toString(),
                                 o.value(QStringLiteral("sensor")).toString(),
                                 o.value(QStringLiteral("value")).toDouble(),
                                 o.value(QStringLiteral("severity")).toString(),
                                 o.value(QStringLiteral("message")).toString(),
                                 o.value(QStringLiteral("timestamp")).toString(),
                                 o.value(QStringLiteral("acknowledged")).toBool());
        }
    });
}

void ApiClient::acknowledgeAlarm(qint64 id) {
    const QString path = QStringLiteral("/api/alarms/%1/ack").arg(id);
    post_json(path, QByteArrayLiteral("{}"), [this, id](int status, const QByteArray&) {
        if (status == 200) {
            Q_EMIT alarmAckSucceeded(id);
        } else {
            Q_EMIT requestFailed(QStringLiteral("Ack failed HTTP %1").arg(status));
        }
    });
}

} // namespace ui
