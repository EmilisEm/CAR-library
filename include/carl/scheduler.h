#pragma once

#include <coroutine>
#include <deque>
#include <mutex>

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

    void schedule(std::coroutine_handle<> handle) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push_back(handle);
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
        while (true) {
            std::coroutine_handle<> handle;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (queue_.empty()) {
                    break;
                }
                handle = queue_.front();
                queue_.pop_front();
            }

            handle.resume();

            if (handle.done()) {
                handle.destroy();
            }
        }
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

private:
    mutable std::mutex mutex_{};
    std::deque<std::coroutine_handle<>> queue_{};
};

}  // namespace carl
