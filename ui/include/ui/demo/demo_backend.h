#pragma once

#include "ui/backend_api.h"
#include "ui/demo/demo_simulator.h"

#include <functional>

namespace ui::demo {

/// BackendApi served from a DemoSimulator instead of the network. Responses
/// are delivered on the next event-loop turn, as a real reply would be, so
/// callers see the same ordering they see against ApiClient.
class DemoBackend : public BackendApi {
    Q_OBJECT

  public:
    explicit DemoBackend(DemoSimulator& simulator, QObject* parent = nullptr);

    bool is_busy() const override { return false; }

    void fetchLatestReadings() override;
    void fetchAlarms() override;
    void bootstrap() override;
    void acknowledgeAlarm(qint64 id) override;
    void fetchHistory(const QString& deviceId, const QString& sensor,
                      const QString& since) override;
    void fetchDevices() override;

    void fetchRules() override;
    void createRule(const QString& deviceId, const QString& sensor, const QString& condition,
                    double threshold, const QString& severity) override;
    void updateRule(qint64 id, const QString& deviceId, const QString& sensor,
                    const QString& condition, double threshold, const QString& severity) override;
    void deleteRule(qint64 id) override;

  private:
    void respond(std::function<void()> deliver);

    DemoSimulator& simulator_;
};

} // namespace ui::demo
