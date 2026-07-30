#include "ingestion/thread_pool.h"

namespace ingestion {

ThreadPool::ThreadPool(std::size_t num_threads) {
    if (num_threads == 0) {
        shutdown_.store(true);
        return;
    }
    workers_.reserve(num_threads);
    for (std::size_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back([this]() { worker_loop(); });
    }
}

ThreadPool::~ThreadPool() {
    shutdown();
}

void ThreadPool::worker_loop() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock lock(mutex_);
            cv_.wait(lock, [this]() { return shutdown_.load() || !tasks_.empty(); });
            if (shutdown_.load() && tasks_.empty())
                return;
            task = std::move(tasks_.front());
            tasks_.pop();
        }
        task();
    }
}

std::future<void> ThreadPool::submit(std::function<void()> task) {
    auto promise = std::make_shared<std::promise<void>>();
    auto future = promise->get_future();
    {
        std::lock_guard lock(mutex_);
        if (shutdown_.load()) {
            throw std::runtime_error("submit on shutdown pool");
        }
        tasks_.emplace([p = std::move(promise), t = std::move(task)]() {
            try {
                t();
                p->set_value();
            } catch (...) {
                p->set_exception(std::current_exception());
            }
        });
    }
    cv_.notify_one();
    return future;
}

void ThreadPool::shutdown() {
    {
        std::lock_guard lock(mutex_);
        if (shutdown_.load())
            return;
        shutdown_.store(true);
    }
    cv_.notify_all();
    for (auto& w : workers_) {
        if (w.joinable())
            w.join();
    }
}

std::size_t ThreadPool::pending_tasks() const {
    std::lock_guard lock(mutex_);
    return tasks_.size();
}

bool ThreadPool::is_shutdown() const {
    return shutdown_.load();
}

} // namespace ingestion
