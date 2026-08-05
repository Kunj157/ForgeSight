#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSignalSpy>
#include <QTimer>
#include <QUrl>
#include <QWebSocket>

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

int http_status(const QUrl& url, const QByteArray& apiKeyHeader = {}) {
    QNetworkAccessManager mgr;
    QNetworkRequest req(url);
    if (!apiKeyHeader.isEmpty()) {
        req.setRawHeader("X-Api-Key", apiKeyHeader);
    }
    auto* reply = mgr.get(req);

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    reply->deleteLater();
    return status;
}

int argc = 0;
QCoreApplication app(argc, nullptr);

} // namespace

TEST(ApiServerTest, StartStop) {
    api::ApiServer server(nullptr);
    EXPECT_TRUE(server.start(0));
    EXPECT_GT(server.port(), 0);
    server.stop();
}

TEST(ApiServerTest, RestartReconnectsMqttWithoutLeakingBridge) {
    // Each start() attempts an MQTT connection (and thus allocates the
    // callback bridge) even when no broker is reachable. Restarting must not
    // leak the previous bridge instance — run under ASan to verify.
    api::ApiServer server(nullptr);
    for (int i = 0; i < 3; ++i) {
        ASSERT_TRUE(server.start(0));
        EXPECT_GT(server.port(), 0);
        server.stop();
    }
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

    QUrl url(QString("http://127.0.0.1:%1/api/history/test-device/temperature").arg(server.port()));
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

TEST(ApiServerTest, HealthEndpointOk) {
    api::ApiServer server(nullptr);
    ASSERT_TRUE(server.start(0));

    QNetworkAccessManager mgr;
    QNetworkRequest req(QUrl(QString("http://127.0.0.1:%1/health").arg(server.port())));
    auto* reply = mgr.get(req);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    EXPECT_EQ(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 200);
    auto obj = QJsonDocument::fromJson(reply->readAll()).object();
    EXPECT_EQ(obj.value("status").toString(), "ok");
    reply->deleteLater();
    server.stop();
}

TEST(ApiServerTest, CreateRuleViaPostRequiresDb) {
    // Without DB, create should return 400.
    api::ApiServer server(nullptr);
    ASSERT_TRUE(server.start(0));

    QNetworkAccessManager mgr;
    QNetworkRequest req(QUrl(QString("http://127.0.0.1:%1/api/rules").arg(server.port())));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QByteArray body =
        R"({"device_id":"pump-001","sensor":"temperature","condition":"gt","threshold":80,"severity":"warning"})";
    auto* reply = mgr.post(req, body);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    EXPECT_EQ(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 400);
    reply->deleteLater();
    server.stop();
}

TEST(ApiServerTest, NoApiKeyConfiguredAllowsRequestsWithoutHeader) {
    // Default/backward-compatible behavior: an empty api key (the default)
    // disables auth entirely, so local dev needs zero extra setup.
    api::ApiServer server(nullptr);
    ASSERT_TRUE(server.start(0));

    QUrl url(QString("http://127.0.0.1:%1/api/devices").arg(server.port()));
    EXPECT_EQ(http_status(url), 200);

    server.stop();
}

TEST(ApiServerTest, ApiRoutesRejectMissingApiKeyWhenConfigured) {
    api::ApiServer server(nullptr);
    ASSERT_TRUE(server.start(0, 0, "", QHostAddress::Any, "secret123"));

    QUrl url(QString("http://127.0.0.1:%1/api/devices").arg(server.port()));
    EXPECT_EQ(http_status(url), 401);

    server.stop();
}

TEST(ApiServerTest, ApiRoutesRejectWrongApiKeyWhenConfigured) {
    api::ApiServer server(nullptr);
    ASSERT_TRUE(server.start(0, 0, "", QHostAddress::Any, "secret123"));

    QUrl url(QString("http://127.0.0.1:%1/api/devices").arg(server.port()));
    EXPECT_EQ(http_status(url, "wrong"), 401);

    server.stop();
}

TEST(ApiServerTest, ApiRoutesAcceptCorrectApiKeyWhenConfigured) {
    api::ApiServer server(nullptr);
    ASSERT_TRUE(server.start(0, 0, "", QHostAddress::Any, "secret123"));

    QUrl url(QString("http://127.0.0.1:%1/api/devices").arg(server.port()));
    EXPECT_EQ(http_status(url, "secret123"), 200);

    server.stop();
}

TEST(ApiServerTest, HealthEndpointStaysOpenEvenWhenApiKeyConfigured) {
    api::ApiServer server(nullptr);
    ASSERT_TRUE(server.start(0, 0, "", QHostAddress::Any, "secret123"));

    QUrl url(QString("http://127.0.0.1:%1/health").arg(server.port()));
    EXPECT_EQ(http_status(url), 200);

    server.stop();
}

TEST(ApiServerTest, WebSocketRejectsWrongApiKeyWhenConfigured) {
    api::ApiServer server(nullptr);
    ASSERT_TRUE(server.start(0, 0, "", QHostAddress::Any, "secret123"));

    QWebSocket socket;
    QSignalSpy disconnectedSpy(&socket, &QWebSocket::disconnected);
    socket.open(QUrl(QString("ws://127.0.0.1:%1/?api_key=wrong").arg(server.wsPort())));

    QEventLoop loop;
    QObject::connect(&socket, &QWebSocket::disconnected, &loop, &QEventLoop::quit);
    QTimer::singleShot(3000, &loop, &QEventLoop::quit);
    loop.exec();

    EXPECT_GE(disconnectedSpy.count(), 1);
    server.stop();
}

TEST(ApiServerTest, WebSocketAcceptsCorrectApiKeyWhenConfigured) {
    api::ApiServer server(nullptr);
    ASSERT_TRUE(server.start(0, 0, "", QHostAddress::Any, "secret123"));

    QWebSocket socket;
    QSignalSpy connectedSpy(&socket, &QWebSocket::connected);
    socket.open(QUrl(QString("ws://127.0.0.1:%1/?api_key=secret123").arg(server.wsPort())));

    QEventLoop loop;
    QObject::connect(&socket, &QWebSocket::connected, &loop, &QEventLoop::quit);
    QTimer::singleShot(3000, &loop, &QEventLoop::quit);
    loop.exec();

    EXPECT_EQ(connectedSpy.count(), 1);
    server.stop();
}

TEST(ApiServerTest, BindAddressCanBeRestrictedToLoopback) {
    api::ApiServer server(nullptr);
    ASSERT_TRUE(server.start(0, 0, "", QHostAddress::LocalHost));
    EXPECT_GT(server.port(), 0);

    QUrl url(QString("http://127.0.0.1:%1/health").arg(server.port()));
    EXPECT_EQ(http_status(url), 200);

    server.stop();
}
