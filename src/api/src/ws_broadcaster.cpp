#include "api/ws_broadcaster.h"

namespace api {

ConnectionId WebSocketBroadcaster::add_connection(WsSendFn send) {
    std::lock_guard lock(mutex_);
    ConnectionId id = next_id_++;
    connections_.emplace(id, std::move(send));
    return id;
}

void WebSocketBroadcaster::remove_connection(ConnectionId id) {
    std::lock_guard lock(mutex_);
    connections_.erase(id);
}

void WebSocketBroadcaster::broadcast(const std::string& message) {
    std::lock_guard lock(mutex_);
    for (const auto& [id, conn] : connections_) {
        conn(message);
    }
}

std::size_t WebSocketBroadcaster::connection_count() const {
    std::lock_guard lock(mutex_);
    return connections_.size();
}

}  // namespace api
