#include "ui/demo/demo_feed.h"

#include <alarm-engine/types.h>

#include <QJsonDocument>
#include <QJsonObject>

namespace ui::demo {

namespace {

constexpr int kPublishIntervalMs = 1000; // config.yaml: publish_interval_sec

QString to_json(const QJsonObject& object) {
    return QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Compact));
}

} // namespace

DemoFeed::DemoFeed(DemoSimulator& simulator, QObject* parent)
    : LiveFeed(parent), simulator_(simulator) {
    timer_.setInterval(kPublishIntervalMs);
    connect(&timer_, &QTimer::timeout, this, [this]() { publish_tick(); });
}

void DemoFeed::set_url(const QUrl& url) {
    if (url_ == url)
        return;
    url_ = url;
    Q_EMIT urlChanged();
}

void DemoFeed::connectToServer() {
    if (connected_)
        return;
    connected_ = true;
    timer_.start();
    Q_EMIT connectedChanged();
}

void DemoFeed::disconnectFromServer() {
    if (!connected_)
        return;
    timer_.stop();
    connected_ = false;
    Q_EMIT connectedChanged();
    Q_EMIT disconnected();
}

void DemoFeed::publish_tick() {
    const auto result = simulator_.tick();

    for (const auto& r : result.readings) {
        QJsonObject o;
        o.insert(QStringLiteral("device_id"), QString::fromStdString(r.device_id));
        o.insert(QStringLiteral("sensor"), QString::fromStdString(r.sensor));
        o.insert(QStringLiteral("value"), r.value);
        o.insert(QStringLiteral("unit"), QString::fromStdString(r.unit));
        o.insert(QStringLiteral("timestamp"), QString::fromStdString(r.timestamp));
        o.insert(QStringLiteral("anomaly"), r.anomaly);
        Q_EMIT readingReceived(to_json(o));
    }

    for (const auto& a : result.new_alarms) {
        QJsonObject o;
        o.insert(QStringLiteral("id"), static_cast<double>(a.id));
        o.insert(QStringLiteral("device_id"), QString::fromStdString(a.device_id));
        o.insert(QStringLiteral("sensor"), QString::fromStdString(a.sensor));
        o.insert(QStringLiteral("value"), a.value);
        o.insert(QStringLiteral("severity"),
                 QString::fromUtf8(alarm_engine::severity_to_string(a.severity)));
        o.insert(QStringLiteral("message"), QString::fromStdString(a.message));
        o.insert(QStringLiteral("timestamp"), QString::fromStdString(a.timestamp));
        o.insert(QStringLiteral("acknowledged"), a.acknowledged);
        Q_EMIT alarmReceived(to_json(o));
    }
}

} // namespace ui::demo
