#pragma once

#include <QObject>
#include <QString>

namespace ui {

/// REST surface the dashboard reads from and writes to. ApiClient implements
/// it against a real ForgeSight backend; the browser demo implements it
/// in-process. The QML layer only sees this interface through the
/// `apiClient` context property, so signal names and payloads here are the
/// contract both implementations must honour.
class BackendApi : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool busy READ is_busy NOTIFY busyChanged)

  public:
    explicit BackendApi(QObject* parent = nullptr) : QObject(parent) {}

    virtual bool is_busy() const = 0;

    Q_INVOKABLE virtual void fetchLatestReadings() = 0;
    Q_INVOKABLE virtual void fetchAlarms() = 0;
    Q_INVOKABLE virtual void bootstrap() = 0;
    Q_INVOKABLE virtual void acknowledgeAlarm(qint64 id) = 0;
    Q_INVOKABLE virtual void fetchHistory(const QString& deviceId, const QString& sensor,
                                          const QString& since) = 0;
    Q_INVOKABLE virtual void fetchDevices() = 0;

    Q_INVOKABLE virtual void fetchRules() = 0;
    Q_INVOKABLE virtual void createRule(const QString& deviceId, const QString& sensor,
                                        const QString& condition, double threshold,
                                        const QString& severity) = 0;
    Q_INVOKABLE virtual void updateRule(qint64 id, const QString& deviceId, const QString& sensor,
                                        const QString& condition, double threshold,
                                        const QString& severity) = 0;
    Q_INVOKABLE virtual void deleteRule(qint64 id) = 0;

  Q_SIGNALS:
    void busyChanged();
    void readingReceived(const QString& deviceId, const QString& sensor, double value,
                         const QString& unit, const QString& timestamp, bool anomaly);
    void alarmReceived(qint64 id, const QString& deviceId, const QString& sensor, double value,
                       const QString& severity, const QString& message, const QString& timestamp,
                       bool acknowledged);
    void alarmAckSucceeded(qint64 id);
    void bootstrapFinished(bool ok, const QString& error);
    void requestFailed(const QString& error);
    void historyPointReceived(const QString& deviceId, const QString& sensor, double value,
                              const QString& unit, const QString& timestamp, bool anomaly);
    void historyLoadFinished(bool ok, const QString& error);
    void deviceMetaReceived(const QString& deviceId, const QString& plant, const QString& floor);
    void ruleReceived(qint64 id, const QString& deviceId, const QString& sensor,
                      const QString& condition, double threshold, const QString& severity);
    void rulesLoadFinished(bool ok, const QString& error);
    void ruleMutationFinished(bool ok, const QString& error);
};

} // namespace ui
