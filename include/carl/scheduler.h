#pragma once

#include <atomic>
#include <condition_variable>
#include <coroutine>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

#include "carl/task.h"

namespace carl {

class Scheduler {
public:
    struct YieldAwaitable {
        Scheduler* scheduler{};

        bool await_ready() const noexcept {
            return false;
        }

        void await_suspend(std::coroutine_handle<> handle) const {
            scheduler->schedule(handle);
        }

        void await_resume() const noexcept {}
    };

    explicit Scheduler(std::size_t worker_count = std::thread::hardware_concurrency()) {
        if (worker_count == 0) {
            worker_count = 1;
        }
        workers_.reserve(worker_count);
        for (std::size_t i = 0; i < worker_count; ++i) {
            workers_.emplace_back([this](std::stop_token stop_token) { worker_loop(stop_token); });
        }
    }

    ~Scheduler() {
        for (auto& worker : workers_) {
            worker.request_stop();
        }
        cv_.notify_all();
    }

    void schedule(std::coroutine_handle<> handle) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push_back(handle);
        }
        cv_.notify_one();
    }

    void spawn(Task task) {
        auto handle = task.release();
        if (handle) {
            schedule(handle);
        }
    }

    YieldAwaitable yield() {
        return YieldAwaitable{this};
    }

    void run() {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this]() { return queue_.empty() && active_.load() == 0; });
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

private:
    void worker_loop(std::stop_token stop_token) {
        while (true) {
            std::coroutine_handle<> handle;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                cv_.wait(lock, [this, &stop_token]() {
                    return stop_token.stop_requested() || !queue_.empty();
                });

                if (stop_token.stop_requested() && queue_.empty()) {
                    return;
                }

                handle = queue_.front();
                queue_.pop_front();
                active_.fetch_add(1);
            }

            handle.resume();

            if (handle.done()) {
                handle.destroy();
            }

            active_.fetch_sub(1);
            cv_.notify_all();
        }
    }

    mutable std::mutex mutex_{};
    std::condition_variable cv_{};
    std::deque<std::coroutine_handle<>> queue_{};
    std::atomic<std::size_t> active_{0};
    std::vector<std::jthread> workers_{};
};

}  // namespace carl
