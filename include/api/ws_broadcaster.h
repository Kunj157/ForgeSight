#pragma once

#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>

namespace api {

using WsSendFn = std::function<void(const std::string&)>;
using ConnectionId = std::uint64_t;

class WebSocketBroadcaster {
public:
    ConnectionId add_connection(WsSendFn send);
    void remove_connection(ConnectionId id);
    void broadcast(const std::string& message);
    std::size_t connection_count() const;

private:
    std::unordered_map<ConnectionId, WsSendFn> connections_;
    ConnectionId next_id_ = 0;
    mutable std::mutex mutex_;
};

}  // namespace api
