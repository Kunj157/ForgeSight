#include <QApplication>
#include <QProcessEnvironment>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QTimer>
#include <QtQml/qqml.h>

#include <cstdio>

#ifdef FORGESIGHT_HAVE_QUICK3D
#include "plant_instancing.h"
#endif

#include "ui/alarm_model.h"
#include "ui/api_client.h"
#include "ui/chart_zoom.h"
#include "ui/device_filter_model.h"
#include "ui/device_model.h"
#include "ui/display_options.h"
#include "ui/history_model.h"
#include "ui/offline_cache.h"
#include "ui/plant_scene_model.h"
#include "ui/rule_model.h"
#include "ui/series_stats.h"
#include "ui/time_format.h"
#include "ui/ws_client.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("ForgeSight");

    std::fprintf(stderr, "ForgeSight: starting (Qt %s)…\n", qVersion());
    std::fflush(stderr);

    ui::DeviceModel deviceModel;
    ui::AlarmModel alarmModel;
    ui::WsClient wsClient;
    ui::HistoryModel historyModel;
    ui::ApiClient apiClient;
    ui::OfflineCache offlineCache;
    ui::TimeFormat timeFormat;
    ui::SeriesStats seriesStats;
    ui::RuleModel ruleModel;
    ui::ChartZoom chartZoom;
    ui::DisplayOptions displayOptions(QProcessEnvironment::systemEnvironment());

    // Declared after every object exposed to QML so it is destroyed first:
    // otherwise live bindings re-evaluate against already-deleted context
    // properties during shutdown ("Cannot read property … of null").
    QQmlApplicationEngine engine;

    // Live per-section views of deviceModel for the device tree.
    qmlRegisterType<ui::DeviceFilterModel>("ForgeSight.Models", 1, 0, "DeviceFilterModel");
    // Live plant-floor layout shared by the 3D view and its 2.5D fallback.
    qmlRegisterType<ui::PlantSceneModel>("ForgeSight.Models", 1, 0, "PlantSceneModel");
#ifdef FORGESIGHT_HAVE_QUICK3D
    qmlRegisterType<ui::PlantInstancing>("ForgeSight.Plant3D", 1, 0, "PlantInstancing");
    constexpr bool quick3dAvailable = true;
#else
    constexpr bool quick3dAvailable = false;
#endif

    QString apiBase = qEnvironmentVariable("FORGESIGHT_API", "http://127.0.0.1:8080");
    QString wsUrl = qEnvironmentVariable("FORGESIGHT_WS", "ws://127.0.0.1:8081");
    QString cacheDbPath =
        qEnvironmentVariable("FORGESIGHT_CACHE_DB", ui::OfflineCache::default_path());
    QString apiKey = qEnvironmentVariable("FORGESIGHT_API_KEY");
    bool allowInsecureTls = qEnvironmentVariable("FORGESIGHT_ALLOW_INSECURE_TLS") == "1";

    apiClient.set_base_url(QUrl(apiBase));
    apiClient.set_api_key(apiKey);
    wsClient.set_url(QUrl(wsUrl));
    wsClient.set_api_key(apiKey);
    wsClient.set_allow_insecure_tls(allowInsecureTls);
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
    // Close the socket (and stop reconnecting) while the event loop is still
    // running, instead of tearing a possibly half-open QWebSocket down from
    // ~WsClient after the QML scene is gone — that path intermittently
    // corrupted the heap on quit after a reconnect.
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &wsClient,
                     [&wsClient]() { wsClient.disconnectFromServer(); });

    auto* ctx = engine.rootContext();
    ctx->setContextProperty("deviceModel", &deviceModel);
    ctx->setContextProperty("alarmModel", &alarmModel);
    ctx->setContextProperty("wsClient", &wsClient);
    ctx->setContextProperty("historyModel", &historyModel);
    ctx->setContextProperty("apiClient", &apiClient);
    ctx->setContextProperty("offlineCache", &offlineCache);
    ctx->setContextProperty("timeFormat", &timeFormat);
    ctx->setContextProperty("seriesStats", &seriesStats);
    // Rules tab. RuleEditorPanel wires apiClient.ruleReceived -> ruleModel
    // itself (it owns the load/clear lifecycle), so no connection is made here
    // — doing both would double-insert each rule.
    ctx->setContextProperty("ruleModel", &ruleModel);
    ctx->setContextProperty("chartZoom", &chartZoom);
    // Startup theme / reduced-motion / 3D preferences (FORGESIGHT_THEME,
    // FORGESIGHT_REDUCED_MOTION, FORGESIGHT_DISABLE_3D).
    ctx->setContextProperty("displayOptions", &displayOptions);
    // Whether this build includes the Qt Quick 3D plant view.
    ctx->setContextProperty("quick3dAvailable", quick3dAvailable);

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
