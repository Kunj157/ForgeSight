#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include "ws_broadcaster.h"

using json = nlohmann::json;
using namespace api;

TEST(WebSocketBroadcasterTest, StartsEmpty) {
    WebSocketBroadcaster broadcaster;
    EXPECT_EQ(broadcaster.connection_count(), 0u);
}

TEST(WebSocketBroadcasterTest, AddConnection) {
    WebSocketBroadcaster broadcaster;
    int call_count = 0;
    broadcaster.add_connection([&call_count](const std::string&) { call_count++; });
    EXPECT_EQ(broadcaster.connection_count(), 1u);
}

TEST(WebSocketBroadcasterTest, BroadcastToAll) {
    WebSocketBroadcaster broadcaster;
    std::string received;

    broadcaster.add_connection([&received](const std::string& msg) { received = msg; });
    broadcaster.add_connection([&received](const std::string& msg) { received = msg; });

    broadcaster.broadcast("hello");
    EXPECT_FALSE(received.empty());
    EXPECT_EQ(received, "hello");
}

TEST(WebSocketBroadcasterTest, RemoveConnection) {
    WebSocketBroadcaster broadcaster;
    int call_count = 0;
    auto id = broadcaster.add_connection([&call_count](const std::string&) { call_count++; });
    EXPECT_EQ(broadcaster.connection_count(), 1u);

    broadcaster.remove_connection(id);
    EXPECT_EQ(broadcaster.connection_count(), 0u);

    broadcaster.broadcast("test");
    EXPECT_EQ(call_count, 0);
}

TEST(WebSocketBroadcasterTest, BroadcastJsonReading) {
    WebSocketBroadcaster broadcaster;
    std::string received;

    broadcaster.add_connection([&received](const std::string& msg) { received = msg; });

    json reading = {
        {"device_id", "pump-001"},
        {"sensor", "temperature"},
        {"value", 65.3},
        {"unit", "°C"},
        {"timestamp", "2026-07-26T10:00:00Z"},
        {"anomaly", false}
    };

    broadcaster.broadcast(reading.dump());

    auto parsed = json::parse(received);
    EXPECT_EQ(parsed["device_id"], "pump-001");
    EXPECT_EQ(parsed["sensor"], "temperature");
    EXPECT_DOUBLE_EQ(parsed["value"].get<double>(), 65.3);
}
