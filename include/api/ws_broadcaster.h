#pragma once

#include <functional>
#include <mutex>
#include <set>
#include <string>

namespace api {

using WsSendFn = std::function<void(const std::string&)>;

class WebSocketBroadcaster {
public:
    void add_connection(WsSendFn send);
    void remove_connection(WsSendFn send);
    void broadcast(const std::string& message);
    std::size_t connection_count() const;

private:
    std::set<WsSendFn> connections_;
    mutable std::mutex mutex_;
};

}  // namespace api
