#pragma once

#include <mutex>
#include <queue>

namespace carl {

template <typename T>
class Mailbox {
public:
    void push(T message) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(std::move(message));
    }

    bool try_pop(T& out) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return false;
        }
        out = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

private:
    mutable std::mutex mutex_{};
    std::queue<T> queue_{};
};

}  // namespace carl
