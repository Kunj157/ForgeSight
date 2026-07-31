#include <gtest/gtest.h>

#include <QCoreApplication>
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
                    buffer_ += sock->readAll();
                    if (!buffer_.contains("\r\n\r\n"))
                        return;

                    const QByteArray req = buffer_;
                    buffer_.clear();
                    last_request_ = req;

                    int status = 200;
                    QByteArray body = QByteArrayLiteral("[]");
                    if (req.contains("POST /api/alarms/") && req.contains("/ack")) {
                        body = QByteArrayLiteral("{\"ok\":true}");
                        status = ack_status_;
                    } else if (req.contains("GET /api/readings/latest")) {
                        body = readings_body_;
                    } else if (req.contains("GET /api/alarms")) {
                        body = alarms_body_;
                    } else if (req.contains("GET /api/history/")) {
                        last_history_request_ = req;
                        body = history_body_;
                    } else if (req.contains("GET /api/devices")) {
                        body = devices_body_;
                    }

                    const QByteArray resp = "HTTP/1.1 " + QByteArray::number(status) +
                                            (status == 200 ? " OK" : " ERR") +
                                            "\r\n"
                                            "Content-Type: application/json\r\n"
                                            "Content-Length: " +
                                            QByteArray::number(body.size()) +
                                            "\r\n"
                                            "Connection: close\r\n\r\n" +
                                            body;
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
        R"([{"id":1,"device_id":"pump-001","sensor":"temperature","value":96,"severity":"critical","message":"hot","timestamp":"2026-07-30T10:00:01Z","acknowledged":true}])";
    QByteArray history_body_ =
        R"([{"device_id":"pump-001","sensor":"temperature","value":65.0,"unit":"C","timestamp":"2026-07-30T09:00:00Z","anomaly":false},)"
        R"({"device_id":"pump-001","sensor":"temperature","value":97.0,"unit":"C","timestamp":"2026-07-30T09:01:00Z","anomaly":true}])";
    QByteArray devices_body_ =
        R"([{"id":"pump-001","name":"pump-001","sensor":"temperature","last_reading_time":"2026-07-30T10:00:00Z","last_value":72.5,"last_unit":"C","anomaly":false,"plant":"Plant A","floor":"Floor 1"}])";
    int ack_status_ = 200;
    QByteArray last_request_;
    QByteArray last_history_request_;

  private:
    QTcpServer server_;
    QByteArray buffer_;
};

} // namespace

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

TEST_F(ApiClientTest, FetchAlarmsEmitsAlarmWithAckState) {
    FakeHttpServer server;
    ui::ApiClient client;
    client.set_base_url(QUrl(QStringLiteral("http://127.0.0.1:%1").arg(server.port())));

    QSignalSpy spy(&client, &ui::ApiClient::alarmReceived);
    client.fetchAlarms();
    ASSERT_TRUE(spy.wait(2000));
    ASSERT_EQ(spy.size(), 1);
    EXPECT_EQ(spy[0][0].toLongLong(), 1);
    EXPECT_EQ(spy[0][4].toString(), "critical");
    EXPECT_TRUE(spy[0][7].toBool());
}

TEST_F(ApiClientTest, FetchHistoryEmitsPointsThenFinished) {
    FakeHttpServer server;
    ui::ApiClient client;
    client.set_base_url(QUrl(QStringLiteral("http://127.0.0.1:%1").arg(server.port())));

    QSignalSpy pointSpy(&client, &ui::ApiClient::historyPointReceived);
    QSignalSpy finishedSpy(&client, &ui::ApiClient::historyLoadFinished);

    client.fetchHistory("pump-001", "temperature", "2026-07-30T00:00:00Z");

    ASSERT_TRUE(finishedSpy.wait(2000));
    ASSERT_EQ(pointSpy.size(), 2);
    EXPECT_EQ(pointSpy[0][0].toString(), "pump-001");
    EXPECT_DOUBLE_EQ(pointSpy[0][2].toDouble(), 65.0);
    EXPECT_TRUE(pointSpy[1][5].toBool());

    ASSERT_EQ(finishedSpy.size(), 1);
    EXPECT_TRUE(finishedSpy[0][0].toBool());
    EXPECT_TRUE(server.last_history_request_.contains(
        "GET /api/history/pump-001/temperature?since=2026-07-30T00:00:00Z"));
}

TEST_F(ApiClientTest, FetchDevicesEmitsDeviceMeta) {
    FakeHttpServer server;
    ui::ApiClient client;
    client.set_base_url(QUrl(QStringLiteral("http://127.0.0.1:%1").arg(server.port())));

    QSignalSpy spy(&client, &ui::ApiClient::deviceMetaReceived);
    client.fetchDevices();
    ASSERT_TRUE(spy.wait(2000));
    ASSERT_EQ(spy.size(), 1);
    EXPECT_EQ(spy[0][0].toString(), "pump-001");
    EXPECT_EQ(spy[0][1].toString(), "Plant A");
    EXPECT_EQ(spy[0][2].toString(), "Floor 1");
}

TEST_F(ApiClientTest, AcknowledgeAlarmPostsAndEmitsSuccess) {
    FakeHttpServer server;
    ui::ApiClient client;
    client.set_base_url(QUrl(QStringLiteral("http://127.0.0.1:%1").arg(server.port())));

    QSignalSpy spy(&client, &ui::ApiClient::alarmAckSucceeded);
    client.acknowledgeAlarm(42);
    ASSERT_TRUE(spy.wait(2000));
    ASSERT_EQ(spy.size(), 1);
    EXPECT_EQ(spy[0][0].toLongLong(), 42);
    EXPECT_TRUE(server.last_request_.contains("POST /api/alarms/42/ack"));
}
