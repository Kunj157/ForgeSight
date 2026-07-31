#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QTimer>

#include <cstdio>

#include "ui/alarm_model.h"
#include "ui/api_client.h"
#include "ui/device_model.h"
#include "ui/history_model.h"
#include "ui/offline_cache.h"
#include "ui/ws_client.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("ForgeSight");

    std::fprintf(stderr, "ForgeSight: starting (Qt %s)…\n", qVersion());
    std::fflush(stderr);

    QQmlApplicationEngine engine;

    ui::DeviceModel deviceModel;
    ui::AlarmModel alarmModel;
    ui::WsClient wsClient;
    ui::HistoryModel historyModel;
    ui::ApiClient apiClient;
    ui::OfflineCache offlineCache;

    QString apiBase = qEnvironmentVariable("FORGESIGHT_API", "http://127.0.0.1:8080");
    QString wsUrl = qEnvironmentVariable("FORGESIGHT_WS", "ws://127.0.0.1:8081");
    QString cacheDbPath =
        qEnvironmentVariable("FORGESIGHT_CACHE_DB", ui::OfflineCache::default_path());

    apiClient.set_base_url(QUrl(apiBase));
    wsClient.set_url(QUrl(wsUrl));
    wsClient.set_auto_reconnect(true);

    if (!offlineCache.open(cacheDbPath)) {
        std::fprintf(stderr, "ForgeSight: could not open offline cache at %s\n",
                     qPrintable(cacheDbPath));
        std::fflush(stderr);
    }

    // Offline-first startup: restore last-known device state from the local
    // cache immediately, before any network I/O completes, so the dashboard
    // is never empty on a cold start with the backend unreachable.
    for (const auto& cached : offlineCache.loadDeviceStates()) {
        deviceModel.updateDeviceMeta(cached.device_id, cached.plant, cached.floor);
        deviceModel.updateDevice(cached.device_id, cached.sensor, cached.value, cached.unit,
                                 cached.timestamp, cached.anomaly);
    }

    // Seed tiles from REST, then keep them live via WebSocket.
    QObject::connect(&apiClient, &ui::ApiClient::readingReceived, &deviceModel,
                     &ui::DeviceModel::updateDevice);
    QObject::connect(&apiClient, &ui::ApiClient::deviceMetaReceived, &deviceModel,
                     &ui::DeviceModel::updateDeviceMeta);
    QObject::connect(&apiClient, &ui::ApiClient::alarmReceived, &alarmModel,
                     [&alarmModel](qint64 id, const QString& deviceId, const QString& sensor,
                                   double value, const QString& severity, const QString& message,
                                   const QString& timestamp, bool acknowledged) {
                         alarmModel.add_alarm(id, deviceId, sensor, value, severity, message,
                                              timestamp, acknowledged);
                     });
    QObject::connect(&apiClient, &ui::ApiClient::alarmAckSucceeded, &alarmModel,
                     &ui::AlarmModel::acknowledge);

    // Every reading (REST bootstrap or live WS push) flows through
    // DeviceModel::updateDevice(), so persisting from its deviceUpdated
    // signal keeps the cache fresh regardless of source.
    QObject::connect(&deviceModel, &ui::DeviceModel::deviceUpdated, &offlineCache,
                     &ui::OfflineCache::saveDeviceState);

    // An ack succeeding clears it from the pending-sync queue whether it was
    // sent live or flushed after reconnecting.
    QObject::connect(&apiClient, &ui::ApiClient::alarmAckSucceeded, &offlineCache,
                     &ui::OfflineCache::clearAck);

    QObject::connect(&wsClient, &ui::WsClient::connectedChanged, &apiClient, [&]() {
        if (wsClient.is_connected()) {
            std::fprintf(stderr, "ForgeSight: WebSocket connected — bootstrapping\n");
            std::fflush(stderr);
            apiClient.bootstrap();

            const auto pending = offlineCache.pendingAcks();
            if (!pending.isEmpty()) {
                std::fprintf(stderr, "ForgeSight: flushing %d queued alarm ack(s)\n",
                             int(pending.size()));
                std::fflush(stderr);
                for (qint64 id : pending) {
                    apiClient.acknowledgeAlarm(id);
                }
            }
        }
    });

    // Also bootstrap once at startup even if WS is down (shows last DB state).
    QTimer::singleShot(300, &apiClient, &ui::ApiClient::bootstrap);

    wsClient.connectToServer();

    auto* ctx = engine.rootContext();
    ctx->setContextProperty("deviceModel", &deviceModel);
    ctx->setContextProperty("alarmModel", &alarmModel);
    ctx->setContextProperty("wsClient", &wsClient);
    ctx->setContextProperty("historyModel", &historyModel);
    ctx->setContextProperty("apiClient", &apiClient);
    ctx->setContextProperty("offlineCache", &offlineCache);

    QObject::connect(&engine, &QQmlEngine::warnings, [](const QList<QQmlError>& warnings) {
        for (const auto& w : warnings) {
            std::fprintf(stderr, "QML: %s\n", qPrintable(w.toString()));
        }
        std::fflush(stderr);
    });

    std::fprintf(stderr, "ForgeSight: loading UI…\n");
    std::fflush(stderr);

    engine.load(QUrl(QStringLiteral("qrc:/app/qml/Main.qml")));
    if (engine.rootObjects().isEmpty()) {
        std::fprintf(stderr, "ForgeSight: UI failed to load (no root objects)\n");
        std::fflush(stderr);
        return -1;
    }

    if (auto* win = qobject_cast<QQuickWindow*>(engine.rootObjects().constFirst())) {
        win->setVisible(true);
        win->show();
        win->raise();
        win->requestActivate();
        std::fprintf(stderr, "ForgeSight: window ready (%dx%d)\n", win->width(), win->height());
        std::fprintf(stderr, "ForgeSight: API %s  WS %s\n", qPrintable(apiBase), qPrintable(wsUrl));
        std::fflush(stderr);
    }

    return app.exec();
}
