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

    QString apiBase = qEnvironmentVariable("FORGESIGHT_API", "http://127.0.0.1:8080");
    QString wsUrl = qEnvironmentVariable("FORGESIGHT_WS", "ws://127.0.0.1:8081");

    apiClient.set_base_url(QUrl(apiBase));
    wsClient.set_url(QUrl(wsUrl));
    wsClient.set_auto_reconnect(true);

    // Seed tiles from REST, then keep them live via WebSocket.
    QObject::connect(&apiClient, &ui::ApiClient::readingReceived, &deviceModel,
                     &ui::DeviceModel::updateDevice);
    QObject::connect(&apiClient, &ui::ApiClient::alarmReceived, &alarmModel,
                     [&alarmModel](qint64 id, const QString& deviceId,
                                   const QString& sensor, double value,
                                   const QString& severity, const QString& message,
                                   const QString& timestamp) {
                         alarmModel.add_alarm(id, deviceId, sensor, value,
                                              severity, message, timestamp);
                     });

    QObject::connect(&wsClient, &ui::WsClient::connectedChanged, &apiClient,
                     [&]() {
                         if (wsClient.is_connected()) {
                             std::fprintf(stderr, "ForgeSight: WebSocket connected — bootstrapping\n");
                             std::fflush(stderr);
                             apiClient.bootstrap();
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

    QObject::connect(&engine, &QQmlEngine::warnings,
                     [](const QList<QQmlError>& warnings) {
                         for (const auto& w : warnings) {
                             std::fprintf(stderr, "QML: %s\n",
                                          qPrintable(w.toString()));
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
        std::fprintf(stderr, "ForgeSight: window ready (%dx%d)\n",
                     win->width(), win->height());
        std::fprintf(stderr, "ForgeSight: API %s  WS %s\n",
                     qPrintable(apiBase), qPrintable(wsUrl));
        std::fflush(stderr);
    }

    return app.exec();
}
