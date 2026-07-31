#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <QUrl>

#include <functional>

namespace ui {

/// Thin REST client used to seed models before/alongside WebSocket live data.
class ApiClient : public QObject {
    Q_OBJECT
    Q_PROPERTY(QUrl baseUrl READ base_url WRITE set_base_url NOTIFY baseUrlChanged)
    Q_PROPERTY(bool busy READ is_busy NOTIFY busyChanged)

  public:
    explicit ApiClient(QObject* parent = nullptr);

    QUrl base_url() const;
    void set_base_url(const QUrl& url);

    bool is_busy() const;

    Q_INVOKABLE void fetchLatestReadings();
    Q_INVOKABLE void fetchAlarms();
    Q_INVOKABLE void bootstrap();
    Q_INVOKABLE void acknowledgeAlarm(qint64 id);
    Q_INVOKABLE void fetchHistory(const QString& deviceId, const QString& sensor,
                                  const QString& since);
    Q_INVOKABLE void fetchDevices();

  Q_SIGNALS:
    void baseUrlChanged();
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

  private:
    void set_busy(bool busy);
    void get_json(const QString& path, const std::function<void(const QByteArray&)>& on_ok);
    void post_json(const QString& path, const QByteArray& body,
                   const std::function<void(int status, const QByteArray&)>& on_done);

    QNetworkAccessManager nam_;
    QUrl base_url_{QStringLiteral("http://127.0.0.1:8080")};
    int pending_ = 0;
    bool busy_ = false;
};

} // namespace ui
