#pragma once

#include "ui/demo/demo_simulator.h"
#include "ui/live_feed.h"

#include <QTimer>

namespace ui::demo {

/// LiveFeed that publishes one DemoSimulator tick per second, in the same
/// JSON shape the backend WebSocket uses, so the dashboard's push handlers
/// run unchanged.
class DemoFeed : public LiveFeed {
    Q_OBJECT

  public:
    explicit DemoFeed(DemoSimulator& simulator, QObject* parent = nullptr);

    bool is_connected() const override { return connected_; }
    QUrl url() const override { return url_; }
    void set_url(const QUrl& url) override;

    void connectToServer() override;
    void disconnectFromServer() override;

  private:
    void publish_tick();

    DemoSimulator& simulator_;
    QTimer timer_;
    QUrl url_{QStringLiteral("demo://simulated")};
    bool connected_ = false;
};

} // namespace ui::demo
