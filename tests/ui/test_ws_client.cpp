#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QSignalSpy>
#include <QTimer>
#include <QWebSocketServer>
#include <QAbstractSocket>

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
    QWebSocketServer server(QStringLiteral("test"),
                            QWebSocketServer::NonSecureMode);
    ASSERT_TRUE(server.listen(QHostAddress::LocalHost, 0));

    ui::WsClient client;
    client.set_auto_reconnect(false);
    client.set_url(QUrl(QStringLiteral("ws://127.0.0.1:%1")
                            .arg(server.serverPort())));

    QSignalSpy connected(&client, &ui::WsClient::connectedChanged);
    client.connectToServer();

    ASSERT_TRUE(connected.wait(2000));
    EXPECT_TRUE(client.is_connected());
}

TEST_F(WsClientTest, ReconnectsAfterServerDrop) {
    QWebSocketServer server(QStringLiteral("test"),
                            QWebSocketServer::NonSecureMode);
    ASSERT_TRUE(server.listen(QHostAddress::LocalHost, 0));

    QWebSocket* peer = nullptr;
    QObject::connect(&server, &QWebSocketServer::newConnection, &server, [&]() {
        peer = server.nextPendingConnection();
    });

    ui::WsClient client;
    client.set_auto_reconnect(true);
    client.set_url(QUrl(QStringLiteral("ws://127.0.0.1:%1")
                            .arg(server.serverPort())));

    QSignalSpy connected(&client, &ui::WsClient::connectedChanged);
    client.connectToServer();
    ASSERT_TRUE(connected.wait(2000));
    ASSERT_TRUE(client.is_connected());
    ASSERT_NE(peer, nullptr);

    // Drop server side — client should reconnect.
    peer->close();
    ASSERT_TRUE(connected.wait(2000));  // disconnected
    EXPECT_FALSE(client.is_connected());

    // Wait for auto-reconnect (delay starts at 1s).
    ASSERT_TRUE(connected.wait(5000));
    EXPECT_TRUE(client.is_connected());
}
