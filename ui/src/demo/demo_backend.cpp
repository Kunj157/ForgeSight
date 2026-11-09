#include "ui/demo/demo_backend.h"

#include <alarm-engine/types.h>

#include <QTimer>

namespace ui::demo {

namespace {

QString qs(const std::string& s) {
    return QString::fromStdString(s);
}

alarm_engine::Rule rule_from_wire(const QString& deviceId, const QString& sensor,
                                  const QString& condition, double threshold,
                                  const QString& severity) {
    alarm_engine::Rule rule;
    rule.device_id = deviceId.toStdString();
    rule.sensor = sensor.toStdString();
    rule.condition = alarm_engine::condition_from_string(condition.toStdString());
    rule.threshold = threshold;
    rule.severity = alarm_engine::severity_from_string(severity.toStdString());
    return rule;
}

} // namespace

DemoBackend::DemoBackend(DemoSimulator& simulator, QObject* parent)
    : BackendApi(parent), simulator_(simulator) {}

void DemoBackend::respond(std::function<void()> deliver) {
    QTimer::singleShot(0, this, std::move(deliver));
}

void DemoBackend::fetchLatestReadings() {
    const auto readings = simulator_.latest_readings();
    respond([this, readings]() {
        for (const auto& r : readings)
            Q_EMIT readingReceived(qs(r.device_id), qs(r.sensor), r.value, qs(r.unit),
                                   qs(r.timestamp), r.anomaly);
    });
}

void DemoBackend::fetchAlarms() {
    const auto alarms = simulator_.alarms();
    respond([this, alarms]() {
        for (const auto& a : alarms)
            Q_EMIT alarmReceived(a.id, qs(a.device_id), qs(a.sensor), a.value,
                                 QString::fromUtf8(alarm_engine::severity_to_string(a.severity)),
                                 qs(a.message), qs(a.timestamp), a.acknowledged);
    });
}

void DemoBackend::fetchDevices() {
    const auto devices = simulator_.devices();
    respond([this, devices]() {
        for (const auto& d : devices)
            Q_EMIT deviceMetaReceived(qs(d.id), qs(d.plant), qs(d.floor));
    });
}

void DemoBackend::bootstrap() {
    fetchLatestReadings();
    fetchAlarms();
    fetchDevices();
    respond([this]() { Q_EMIT bootstrapFinished(true, QString()); });
}

void DemoBackend::acknowledgeAlarm(qint64 id) {
    const bool ok = simulator_.acknowledge(id);
    respond([this, id, ok]() {
        if (ok)
            Q_EMIT alarmAckSucceeded(id);
        else
            Q_EMIT requestFailed(QStringLiteral("Ack failed HTTP 404"));
    });
}

void DemoBackend::fetchHistory(const QString& deviceId, const QString& sensor,
                               const QString& since) {
    const auto points =
        simulator_.history(deviceId.toStdString(), sensor.toStdString(), since.toStdString());
    respond([this, points]() {
        for (const auto& p : points)
            Q_EMIT historyPointReceived(qs(p.device_id), qs(p.sensor), p.value, qs(p.unit),
                                        qs(p.timestamp), p.anomaly);
        Q_EMIT historyLoadFinished(true, QString());
    });
}

void DemoBackend::fetchRules() {
    const auto rules = simulator_.rules();
    respond([this, rules]() {
        for (const auto& r : rules)
            Q_EMIT ruleReceived(r.id, qs(r.device_id), qs(r.sensor),
                                QString::fromUtf8(alarm_engine::condition_to_string(r.condition)),
                                r.threshold,
                                QString::fromUtf8(alarm_engine::severity_to_string(r.severity)));
        Q_EMIT rulesLoadFinished(true, QString());
    });
}

void DemoBackend::createRule(const QString& deviceId, const QString& sensor,
                             const QString& condition, double threshold, const QString& severity) {
    simulator_.add_rule(rule_from_wire(deviceId, sensor, condition, threshold, severity));
    respond([this]() { Q_EMIT ruleMutationFinished(true, QString()); });
}

void DemoBackend::updateRule(qint64 id, const QString& deviceId, const QString& sensor,
                             const QString& condition, double threshold, const QString& severity) {
    const bool ok = simulator_.update_rule(
        id, rule_from_wire(deviceId, sensor, condition, threshold, severity));
    respond([this, ok]() {
        Q_EMIT ruleMutationFinished(ok, ok ? QString() : QStringLiteral("HTTP 404"));
    });
}

void DemoBackend::deleteRule(qint64 id) {
    const bool ok = simulator_.delete_rule(id);
    respond([this, ok]() {
        Q_EMIT ruleMutationFinished(ok, ok ? QString() : QStringLiteral("HTTP 404"));
    });
}

} // namespace ui::demo
