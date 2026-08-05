#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <numeric>
#include <thread>
#include <vector>

#include "ingestion/thread_pool.h"

using namespace ingestion;

TEST(ThreadPoolTest, ConstructWithZeroThreads) {
    ThreadPool pool(0);
    EXPECT_TRUE(pool.is_shutdown());
}

TEST(ThreadPoolTest, SubmitTask) {
    ThreadPool pool(2);
    std::atomic<int> counter{0};
    auto fut = pool.submit([&counter]() { counter.fetch_add(1); });
    fut.get();
    EXPECT_EQ(counter.load(), 1);
}

TEST(ThreadPoolTest, MultipleTasks) {
    ThreadPool pool(4);
    std::atomic<int> counter{0};
    constexpr int kTasks = 100;

    std::vector<std::future<void>> futures;
    futures.reserve(kTasks);
    for (int i = 0; i < kTasks; ++i) {
        futures.push_back(
            pool.submit([&counter]() { counter.fetch_add(1, std::memory_order_relaxed); }));
    }
    for (auto& f : futures) {
        f.get();
    }
    EXPECT_EQ(counter.load(), kTasks);
}

TEST(ThreadPoolTest, ShutdownDrainsQueue) {
    ThreadPool pool(4);
    std::atomic<int> counter{0};

    for (int i = 0; i < 10; ++i) {
        pool.submit([&counter]() { counter.fetch_add(1, std::memory_order_relaxed); });
    }

    pool.shutdown();
    EXPECT_TRUE(pool.is_shutdown());
    EXPECT_EQ(counter.load(), 10);
}

TEST(ThreadPoolTest, PendingTasksCount) {
    ThreadPool pool(1);
    std::atomic<bool> gate{false};
    std::atomic<bool> release{false};

    pool.submit([&gate, &release]() {
        gate.store(true);
        while (!release.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

    while (!gate.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    auto f = pool.submit([]() {});
    EXPECT_GE(pool.pending_tasks(), 1u);

    release.store(true);
    f.get();
    pool.shutdown();
}

TEST(ThreadPoolTest, TaskExceptionPropagates) {
    ThreadPool pool(1);
    auto fut = pool.submit([]() { throw std::runtime_error("test error"); });
    EXPECT_THROW(fut.get(), std::runtime_error);
}
