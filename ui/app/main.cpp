#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "ui/alarm_model.h"
#include "ui/device_model.h"
#include "ui/ws_client.h"
#include "ui/history_model.h"
#include "ui/csv_export.h"
#include "ui/pdf_export.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("ForgeSight");

    QQmlApplicationEngine engine;

    ui::DeviceModel deviceModel;
    ui::AlarmModel alarmModel;
    ui::WsClient wsClient;
    ui::HistoryModel historyModel;

    wsClient.set_url(QUrl("ws://127.0.0.1:8081"));
    wsClient.connectToServer();

    auto* ctx = engine.rootContext();
    ctx->setContextProperty("deviceModel", &deviceModel);
    ctx->setContextProperty("alarmModel", &alarmModel);
    ctx->setContextProperty("wsClient", &wsClient);
    ctx->setContextProperty("historyModel", &historyModel);

    engine.load(QUrl("qrc:/app/qml/Main.qml"));
    if (engine.rootObjects().isEmpty()) return -1;

    return app.exec();
}
