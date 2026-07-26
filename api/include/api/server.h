#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "api/device_service.h"
#include "api/rule_service.h"
#include "api/ws_broadcaster.h"

namespace api {

class Server {
public:
    Server(DeviceService& devices, RuleService& rules,
           WebSocketBroadcaster& broadcaster);
    ~Server();

    void run(std::uint16_t port);
    void stop();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace api
