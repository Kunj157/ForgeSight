#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "ui/alarm_model.h"
#include "ui/device_model.h"
#include "ui/ws_client.h"

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    app.setApplicationName("ForgeSight");

    QQmlApplicationEngine engine;

    ui::DeviceModel deviceModel;
    ui::AlarmModel alarmModel;
    ui::WsClient wsClient;

    auto* ctx = engine.rootContext();
    ctx->setContextProperty("deviceModel", &deviceModel);
    ctx->setContextProperty("alarmModel", &alarmModel);
    ctx->setContextProperty("wsClient", &wsClient);

    engine.loadFromModule("app", "Main");
    if (engine.rootObjects().isEmpty()) return -1;

    return app.exec();
}
