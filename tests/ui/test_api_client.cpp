#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>

#include "ui/api_client.h"

namespace {

class FakeHttpServer : public QObject {
public:
    explicit FakeHttpServer(QObject* parent = nullptr) : QObject(parent) {
        server_.listen(QHostAddress::LocalHost, 0);
        QObject::connect(&server_, &QTcpServer::newConnection, this, [this]() {
            while (server_.hasPendingConnections()) {
                auto* sock = server_.nextPendingConnection();
                QObject::connect(sock, &QTcpSocket::readyRead, sock, [this, sock]() {
                    const QByteArray req = sock->readAll();
                    QByteArray body = QByteArrayLiteral("[]");
                    if (req.contains("GET /api/readings/latest")) {
                        body = readings_body_;
                    } else if (req.contains("GET /api/alarms")) {
                        body = alarms_body_;
                    }
                    const QByteArray resp =
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: application/json\r\n"
                        "Content-Length: " + QByteArray::number(body.size()) + "\r\n"
                        "Connection: close\r\n\r\n" + body;
                    sock->write(resp);
                    sock->disconnectFromHost();
                });
            }
        });
    }

    quint16 port() const { return server_.serverPort(); }

    QByteArray readings_body_ =
        R"([{"device_id":"pump-001","sensor":"temperature","value":72.5,"unit":"C","timestamp":"2026-07-30T10:00:00Z","anomaly":false}])";
    QByteArray alarms_body_ =
        R"([{"id":1,"device_id":"pump-001","sensor":"temperature","value":96,"severity":"critical","message":"hot","timestamp":"2026-07-30T10:00:01Z"}])";

private:
    QTcpServer server_;
};

}  // namespace

class ApiClientTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        static int argc = 0;
        static char* argv[] = {nullptr};
        static QCoreApplication app(argc, argv);
    }
};

TEST_F(ApiClientTest, FetchLatestReadingsEmitsReading) {
    FakeHttpServer server;
    ui::ApiClient client;
    client.set_base_url(QUrl(QStringLiteral("http://127.0.0.1:%1").arg(server.port())));

    QSignalSpy spy(&client, &ui::ApiClient::readingReceived);
    client.fetchLatestReadings();
    ASSERT_TRUE(spy.wait(2000));
    ASSERT_EQ(spy.size(), 1);
    EXPECT_EQ(spy[0][0].toString(), "pump-001");
    EXPECT_EQ(spy[0][1].toString(), "temperature");
    EXPECT_DOUBLE_EQ(spy[0][2].toDouble(), 72.5);
}

TEST_F(ApiClientTest, FetchAlarmsEmitsAlarm) {
    FakeHttpServer server;
    ui::ApiClient client;
    client.set_base_url(QUrl(QStringLiteral("http://127.0.0.1:%1").arg(server.port())));

    QSignalSpy spy(&client, &ui::ApiClient::alarmReceived);
    client.fetchAlarms();
    ASSERT_TRUE(spy.wait(2000));
    ASSERT_EQ(spy.size(), 1);
    EXPECT_EQ(spy[0][0].toLongLong(), 1);
    EXPECT_EQ(spy[0][4].toString(), "critical");
}
