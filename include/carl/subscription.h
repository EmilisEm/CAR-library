#pragma once

#include <functional>
#include <utility>

namespace carl {

class Subscription {
public:
    Subscription() = default;
    explicit Subscription(std::function<void()> cancel) : cancel_(std::move(cancel)) {}

    Subscription(Subscription&& other) noexcept : cancel_(std::exchange(other.cancel_, {})) {}
    Subscription& operator=(Subscription&& other) noexcept {
        if (this != &other) {
            unsubscribe();
            cancel_ = std::exchange(other.cancel_, {});
        }
        return *this;
    }

    Subscription(const Subscription&) = delete;
    Subscription& operator=(const Subscription&) = delete;

    ~Subscription() {
        unsubscribe();
    }

    void unsubscribe() {
        if (cancel_) {
            cancel_();
            cancel_ = nullptr;
        }
    }

    explicit operator bool() const noexcept {
        return static_cast<bool>(cancel_);
    }

private:
    std::function<void()> cancel_{};
};

}  // namespace carl
