#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include "event_bus.h"
#include "reading.h"

using namespace ingestion;

TEST(EventBusTest, SingleSubscriberReceivesEvent) {
    EventBus bus;
    ReadingEvent received;
    bool called = false;

    bus.subscribe([&](const ReadingEvent& e) {
        received = e;
        called = true;
    });

    Reading r;
    r.device_id = "pump-001";
    r.sensor = "temperature";
    r.value = 65.0;

    bus.publish({r, std::chrono::system_clock::now()});

    EXPECT_TRUE(called);
    EXPECT_EQ(received.reading.device_id, "pump-001");
    EXPECT_DOUBLE_EQ(received.reading.value, 65.0);
}

TEST(EventBusTest, MultipleSubscribers) {
    EventBus bus;
    std::atomic<int> count{0};

    bus.subscribe([&](const ReadingEvent&) { count.fetch_add(1); });
    bus.subscribe([&](const ReadingEvent&) { count.fetch_add(1); });
    bus.subscribe([&](const ReadingEvent&) { count.fetch_add(1); });

    Reading r;
    r.device_id = "a";
    r.sensor = "b";
    bus.publish({r, std::chrono::system_clock::now()});

    EXPECT_EQ(count.load(), 3);
}

TEST(EventBusTest, NoSubscribersNoCrash) {
    EventBus bus;
    Reading r;
    r.device_id = "a";
    r.sensor = "b";
    bus.publish({r, std::chrono::system_clock::now()});
}

TEST(EventBusTest, SubscriberCount) {
    EventBus bus;
    EXPECT_EQ(bus.subscriber_count(), 0u);
    bus.subscribe([](const ReadingEvent&) {});
    EXPECT_EQ(bus.subscriber_count(), 1u);
    bus.subscribe([](const ReadingEvent&) {});
    EXPECT_EQ(bus.subscriber_count(), 2u);
}

TEST(EventBusTest, ThreadSafePublish) {
    EventBus bus;
    std::atomic<int> count{0};

    bus.subscribe([&](const ReadingEvent&) { count.fetch_add(1); });

    std::vector<std::thread> threads;
    constexpr int kThreads = 8;
    constexpr int kPerThread = 100;

    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&bus]() {
            for (int j = 0; j < kPerThread; ++j) {
                Reading r;
                r.device_id = "a";
                r.sensor = "b";
                bus.publish({r, std::chrono::system_clock::now()});
            }
        });
    }

    for (auto& t : threads) t.join();
    EXPECT_EQ(count.load(), kThreads * kPerThread);
}
