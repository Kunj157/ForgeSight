#include <gtest/gtest.h>

#include <QAbstractSocket>
#include <QCoreApplication>
#include <QSignalSpy>
#include <QUrlQuery>
#include <QWebSocket>
#include <QWebSocketServer>

#include "ui/ws_client.h"

class WsClientTest : public ::testing::Test {
  protected:
    static void SetUpTestSuite() {
        static int argc = 0;
        static char* argv[] = {nullptr};
        static QCoreApplication app(argc, argv);
    }
};

TEST_F(WsClientTest, ReconnectDelayBacksOff) {
    EXPECT_EQ(ui::WsClient::reconnect_delay_ms(0), 1000);
    EXPECT_EQ(ui::WsClient::reconnect_delay_ms(1), 2000);
    EXPECT_EQ(ui::WsClient::reconnect_delay_ms(2), 4000);
    EXPECT_EQ(ui::WsClient::reconnect_delay_ms(10), 30000);
}

TEST_F(WsClientTest, AutoReconnectEnabledByDefault) {
    ui::WsClient client;
    EXPECT_TRUE(client.auto_reconnect());
}

TEST_F(WsClientTest, ConnectsToLocalServer) {
    QWebSocketServer server(QStringLiteral("test"), QWebSocketServer::NonSecureMode);
    ASSERT_TRUE(server.listen(QHostAddress::LocalHost, 0));

    QObject::connect(&server, &QWebSocketServer::newConnection, &server, [&server]() {
        if (auto* sock = server.nextPendingConnection())
            sock->setParent(&server);
    });

    ui::WsClient client;
    client.set_auto_reconnect(false);
    client.set_url(QUrl(QStringLiteral("ws://127.0.0.1:%1").arg(server.serverPort())));

    QSignalSpy connected(&client, &ui::WsClient::connectedChanged);
    client.connectToServer();

    ASSERT_TRUE(connected.wait(2000));
    EXPECT_TRUE(client.is_connected());

    client.disconnectFromServer();
    if (client.is_connected())
        ASSERT_TRUE(connected.wait(2000));
    EXPECT_FALSE(client.is_connected());
    server.close();
}

TEST_F(WsClientTest, DoesNotAppendApiKeyQueryParamByDefault) {
    QWebSocketServer server(QStringLiteral("test"), QWebSocketServer::NonSecureMode);
    ASSERT_TRUE(server.listen(QHostAddress::LocalHost, 0));

    QUrl capturedUrl;
    QObject::connect(&server, &QWebSocketServer::newConnection, &server, [&server, &capturedUrl]() {
        if (auto* sock = server.nextPendingConnection()) {
            capturedUrl = sock->requestUrl();
            sock->setParent(&server);
        }
    });

    ui::WsClient client;
    client.set_auto_reconnect(false);
    client.set_url(QUrl(QStringLiteral("ws://127.0.0.1:%1").arg(server.serverPort())));

    QSignalSpy connected(&client, &ui::WsClient::connectedChanged);
    client.connectToServer();
    ASSERT_TRUE(connected.wait(2000));

    EXPECT_TRUE(QUrlQuery(capturedUrl.query()).queryItemValue("api_key").isEmpty());
}

TEST_F(WsClientTest, AppendsApiKeyQueryParamWhenConfigured) {
    QWebSocketServer server(QStringLiteral("test"), QWebSocketServer::NonSecureMode);
    ASSERT_TRUE(server.listen(QHostAddress::LocalHost, 0));

    QUrl capturedUrl;
    QObject::connect(&server, &QWebSocketServer::newConnection, &server, [&server, &capturedUrl]() {
        if (auto* sock = server.nextPendingConnection()) {
            capturedUrl = sock->requestUrl();
            sock->setParent(&server);
        }
    });

    ui::WsClient client;
    client.set_auto_reconnect(false);
    client.set_api_key(QStringLiteral("secret123"));
    client.set_url(QUrl(QStringLiteral("ws://127.0.0.1:%1").arg(server.serverPort())));

    QSignalSpy connected(&client, &ui::WsClient::connectedChanged);
    client.connectToServer();
    ASSERT_TRUE(connected.wait(2000));

    EXPECT_EQ(QUrlQuery(capturedUrl.query()).queryItemValue("api_key"), "secret123");
    // The public `url` property should still reflect the configured base URL,
    // not the key-augmented one actually used on the wire.
    EXPECT_TRUE(client.url().query().isEmpty());
}

TEST_F(WsClientTest, AllowInsecureTlsDisabledByDefault) {
    ui::WsClient client;
    EXPECT_FALSE(client.allow_insecure_tls());
}

TEST_F(WsClientTest, AllowInsecureTlsCanBeEnabledExplicitly) {
    ui::WsClient client;
    client.set_allow_insecure_tls(true);
    EXPECT_TRUE(client.allow_insecure_tls());
}

TEST_F(WsClientTest, ReconnectsAfterServerDrop) {
    QWebSocketServer server(QStringLiteral("test"), QWebSocketServer::NonSecureMode);
    ASSERT_TRUE(server.listen(QHostAddress::LocalHost, 0));

    QObject::connect(&server, &QWebSocketServer::newConnection, &server, [&server]() {
        if (auto* sock = server.nextPendingConnection())
            sock->setParent(&server);
    });

    ui::WsClient client;
    client.set_auto_reconnect(true);
    client.set_url(QUrl(QStringLiteral("ws://127.0.0.1:%1").arg(server.serverPort())));

    QSignalSpy connected(&client, &ui::WsClient::connectedChanged);
    client.connectToServer();
    ASSERT_TRUE(connected.wait(2000));
    ASSERT_TRUE(client.is_connected());

    auto peers = server.findChildren<QWebSocket*>();
    ASSERT_FALSE(peers.isEmpty());
    peers.first()->abort();

    ASSERT_TRUE(connected.wait(2000));
    EXPECT_FALSE(client.is_connected());

    ASSERT_TRUE(connected.wait(5000));
    EXPECT_TRUE(client.is_connected());

    client.set_auto_reconnect(false);
    client.disconnectFromServer();
    connected.wait(1000);
    server.close();
    QCoreApplication::processEvents();
}
