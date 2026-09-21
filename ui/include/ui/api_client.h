#pragma once

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QObject>
#include <QString>
#include <QUrl>

#include <functional>

QT_FORWARD_DECLARE_CLASS(QNetworkReply)

namespace ui {

/// Thin REST client used to seed models before/alongside WebSocket live data.
class ApiClient : public QObject {
    Q_OBJECT
    Q_PROPERTY(QUrl baseUrl READ base_url WRITE set_base_url NOTIFY baseUrlChanged)
    Q_PROPERTY(bool busy READ is_busy NOTIFY busyChanged)
    Q_PROPERTY(QString apiKey READ api_key WRITE set_api_key NOTIFY apiKeyChanged)

  public:
    explicit ApiClient(QObject* parent = nullptr);

    QUrl base_url() const;
    void set_base_url(const QUrl& url);

    bool is_busy() const;

    QString api_key() const;
    void set_api_key(const QString& key);

    Q_INVOKABLE void fetchLatestReadings();
    Q_INVOKABLE void fetchAlarms();
    Q_INVOKABLE void bootstrap();
    Q_INVOKABLE void acknowledgeAlarm(qint64 id);
    Q_INVOKABLE void fetchHistory(const QString& deviceId, const QString& sensor,
                                  const QString& since);
    Q_INVOKABLE void fetchDevices();

    Q_INVOKABLE void fetchRules();
    Q_INVOKABLE void createRule(const QString& deviceId, const QString& sensor,
                                const QString& condition, double threshold,
                                const QString& severity);
    Q_INVOKABLE void updateRule(qint64 id, const QString& deviceId, const QString& sensor,
                                const QString& condition, double threshold,
                                const QString& severity);
    Q_INVOKABLE void deleteRule(qint64 id);

  Q_SIGNALS:
    void baseUrlChanged();
    void busyChanged();
    void apiKeyChanged();
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

  private:
    void set_busy(bool busy);
    void get_json(const QString& path, const std::function<void(const QByteArray&)>& on_ok);
    void post_json(const QString& path, const QByteArray& body,
                   const std::function<void(int status, const QByteArray&)>& on_done);
    // Common completion handling for a rule create/update/delete: emits
    // ruleMutationFinished(ok, error) once `reply` finishes. Shared so all
    // three verbs report success/failure identically.
    void handle_rule_mutation(QNetworkReply* reply);
    QNetworkRequest make_request(const QUrl& url) const;

    QNetworkAccessManager nam_;
    QUrl base_url_{QStringLiteral("http://127.0.0.1:8080")};
    QString api_key_;
    int pending_ = 0;
    bool busy_ = false;
};

} // namespace ui
