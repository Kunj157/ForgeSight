#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QEventLoop>

#include "api/api_server.h"

namespace {

QJsonArray get_json(const QUrl& url) {
    QNetworkAccessManager mgr;
    QNetworkRequest req(url);
    auto* reply = mgr.get(req);

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    auto data = reply->readAll();
    reply->deleteLater();

    return QJsonDocument::fromJson(data).array();
}

int argc = 0;
QCoreApplication app(argc, nullptr);

}  // namespace

TEST(ApiServerTest, StartStop) {
    api::ApiServer server(nullptr);
    EXPECT_TRUE(server.start(0));
    EXPECT_GT(server.port(), 0);
    server.stop();
}

TEST(ApiServerTest, DevicesEndpointReturnsArray) {
    api::ApiServer server(nullptr);
    ASSERT_TRUE(server.start(0));

    QUrl url(QString("http://127.0.0.1:%1/api/devices").arg(server.port()));
    auto devices = get_json(url);

    EXPECT_TRUE(devices.isEmpty());

    server.stop();
}

TEST(ApiServerTest, HistoryEndpointReturnsArray) {
    api::ApiServer server(nullptr);
    ASSERT_TRUE(server.start(0));

    QUrl url(QString("http://127.0.0.1:%1/api/history/test-device/temperature")
                .arg(server.port()));
    auto readings = get_json(url);

    EXPECT_TRUE(readings.isEmpty());

    server.stop();
}

TEST(ApiServerTest, HistoryEndpointWithSince) {
    api::ApiServer server(nullptr);
    ASSERT_TRUE(server.start(0));

    QUrl url(QString("http://127.0.0.1:%1/api/history/test-device/temperature"
                     "?since=2026-01-01T00:00:00Z")
                .arg(server.port()));
    auto readings = get_json(url);

    server.stop();
}

TEST(ApiServerTest, RulesEndpointReturnsArray) {
    api::ApiServer server(nullptr);
    ASSERT_TRUE(server.start(0));

    QUrl url(QString("http://127.0.0.1:%1/api/rules").arg(server.port()));
    auto rules = get_json(url);

    EXPECT_TRUE(rules.isEmpty());

    server.stop();
}
