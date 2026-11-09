#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QMap>
#include <QMetaObject>
#include <QSignalSpy>

#include "ui/api_client.h"
#include "ui/backend_api.h"
#include "ui/live_feed.h"
#include "ui/ws_client.h"

namespace {

// No Q_OBJECT here: this directory does not run AUTOMOC, and invokeMethod
// dispatches through BackendApi's metaobject, which calls the overrides.
// Stands in for the browser demo: every REST call is recorded and no network
// is touched, so the test exercises only the interface contract.
class FakeBackend : public ui::BackendApi {
  public:
    explicit FakeBackend(QObject* parent = nullptr) : ui::BackendApi(parent) {}

    bool is_busy() const override { return false; }
    void fetchLatestReadings() override { ++calls["fetchLatestReadings"]; }
    void fetchAlarms() override { ++calls["fetchAlarms"]; }
    void bootstrap() override { ++calls["bootstrap"]; }
    void acknowledgeAlarm(qint64 id) override {
        ++calls["acknowledgeAlarm"];
        last_id = id;
        Q_EMIT alarmAckSucceeded(id);
    }
    void fetchHistory(const QString&, const QString&, const QString&) override {
        ++calls["fetchHistory"];
    }
    void fetchDevices() override { ++calls["fetchDevices"]; }
    void fetchRules() override { ++calls["fetchRules"]; }
    void createRule(const QString&, const QString&, const QString&, double,
                    const QString&) override {
        ++calls["createRule"];
    }
    void updateRule(qint64, const QString&, const QString&, const QString&, double,
                    const QString&) override {
        ++calls["updateRule"];
    }
    void deleteRule(qint64) override { ++calls["deleteRule"]; }

    QMap<QString, int> calls;
    qint64 last_id = 0;
};

class FakeFeed : public ui::LiveFeed {
  public:
    explicit FakeFeed(QObject* parent = nullptr) : ui::LiveFeed(parent) {}

    bool is_connected() const override { return connected_; }
    QUrl url() const override { return url_; }
    void set_url(const QUrl& url) override {
        url_ = url;
        Q_EMIT urlChanged();
    }
    void connectToServer() override {
        connected_ = true;
        Q_EMIT connectedChanged();
    }
    void disconnectFromServer() override {
        connected_ = false;
        Q_EMIT connectedChanged();
    }

  private:
    bool connected_ = false;
    QUrl url_;
};

} // namespace

// The REST surface must stay reachable by name from QML (`apiClient.fetchRules()`,
// `apiClient.readingReceived`, ...) on the concrete ApiClient, now that the
// methods and signals live on BackendApi.
TEST(DataSourceSeam, ApiClientExposesBackendSurfaceByName) {
    const QMetaObject* mo = &ui::ApiClient::staticMetaObject;
    EXPECT_TRUE(mo->inherits(&ui::BackendApi::staticMetaObject));
    EXPECT_GE(mo->indexOfMethod("fetchHistory(QString,QString,QString)"), 0);
    EXPECT_GE(mo->indexOfMethod("createRule(QString,QString,QString,double,QString)"), 0);
    EXPECT_GE(mo->indexOfSignal("readingReceived(QString,QString,double,QString,QString,bool)"), 0);
    EXPECT_GE(mo->indexOfSignal("ruleMutationFinished(bool,QString)"), 0);
    EXPECT_GE(mo->indexOfProperty("busy"), 0);
}

TEST(DataSourceSeam, WsClientExposesLiveFeedSurfaceByName) {
    const QMetaObject* mo = &ui::WsClient::staticMetaObject;
    EXPECT_TRUE(mo->inherits(&ui::LiveFeed::staticMetaObject));
    EXPECT_GE(mo->indexOfProperty("connected"), 0);
    EXPECT_GE(mo->indexOfProperty("url"), 0);
    EXPECT_GE(mo->indexOfSignal("readingReceived(QString)"), 0);
    EXPECT_GE(mo->indexOfSignal("alarmReceived(QString)"), 0);
    // Concrete-only settings stay on WsClient.
    EXPECT_GE(mo->indexOfProperty("autoReconnect"), 0);
    EXPECT_GE(mo->indexOfProperty("allowInsecureTls"), 0);
}

TEST(DataSourceSeam, ConcreteClientsAreUsableThroughInterfaces) {
    ui::ApiClient api;
    ui::BackendApi* backend = &api;
    EXPECT_FALSE(backend->is_busy());

    ui::WsClient ws;
    ui::LiveFeed* feed = &ws;
    EXPECT_FALSE(feed->is_connected());
    feed->set_url(QUrl(QStringLiteral("ws://127.0.0.1:9")));
    EXPECT_EQ(feed->url(), QUrl(QStringLiteral("ws://127.0.0.1:9")));
    EXPECT_EQ(ws.url(), feed->url());
}

TEST(DataSourceSeam, InvokableCallsDispatchToOverride) {
    FakeBackend backend;
    // Dynamic invocation is how QML reaches the methods; it must land on the
    // subclass override, not on a BackendApi default.
    QMetaObject::invokeMethod(&backend, "fetchRules");
    QMetaObject::invokeMethod(&backend, "acknowledgeAlarm", Q_ARG(qint64, 42));
    EXPECT_EQ(backend.calls.value("fetchRules"), 1);
    EXPECT_EQ(backend.calls.value("acknowledgeAlarm"), 1);
    EXPECT_EQ(backend.last_id, 42);
}

TEST(DataSourceSeam, AckSignalReachesBaseTypedConnection) {
    FakeBackend backend;
    QSignalSpy acks(&backend, &ui::BackendApi::alarmAckSucceeded);
    ui::BackendApi* base = &backend;
    base->acknowledgeAlarm(7);
    ASSERT_EQ(acks.count(), 1);
    EXPECT_EQ(acks.first().first().toLongLong(), 7);
}

TEST(DataSourceSeam, LiveFeedSignalsAndPropertiesWorkThroughBasePointer) {
    FakeFeed fake;
    ui::LiveFeed* feed = &fake;
    QSignalSpy connected(feed, &ui::LiveFeed::connectedChanged);
    QSignalSpy urls(feed, &ui::LiveFeed::urlChanged);

    feed->set_url(QUrl(QStringLiteral("ws://demo")));
    feed->connectToServer();
    EXPECT_TRUE(feed->property("connected").toBool());
    EXPECT_EQ(feed->property("url").toUrl(), QUrl(QStringLiteral("ws://demo")));
    EXPECT_EQ(urls.count(), 1);
    EXPECT_EQ(connected.count(), 1);

    feed->disconnectFromServer();
    EXPECT_FALSE(feed->property("connected").toBool());
    EXPECT_EQ(connected.count(), 2);
}
