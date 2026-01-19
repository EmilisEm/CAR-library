#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

#include "carl/subscription.h"

namespace carl {

template <typename T>
class Signal {
public:
    using Callback = std::function<void(const T&)>;

    Signal() = delete;
    explicit Signal(T initial)
        : state_(std::make_shared<State>(std::move(initial))),
          ownership_(std::make_shared<Ownership>()) {}

    T value() const {
        std::lock_guard<std::mutex> lock(state_->mutex);
        return state_->value;
    }

    void set(T value) {
        std::vector<Callback> callbacks;
        T current;
        {
            std::lock_guard<std::mutex> lock(state_->mutex);
            state_->value = std::move(value);
            current = state_->value;
            callbacks = state_->observers;
        }

        for (const auto& callback : callbacks) {
            if (callback) {
                callback(current);
            }
        }
    }

    Subscription subscribe(Callback callback) {
        std::lock_guard<std::mutex> lock(state_->mutex);
        const std::size_t index = state_->observers.size();
        state_->observers.emplace_back(std::move(callback));

        std::weak_ptr<State> weak_state = state_;
        return Subscription([weak_state, index]() {
            if (auto shared_state = weak_state.lock()) {
                std::lock_guard<std::mutex> guard(shared_state->mutex);
                if (index < shared_state->observers.size()) {
                    shared_state->observers[index] = nullptr;
                }
            }
        });
    }

    void keep_alive(Subscription subscription) {
        std::lock_guard<std::mutex> lock(ownership_->mutex);
        ownership_->subscriptions.emplace_back(std::move(subscription));
    }

private:
    struct State {
        explicit State(T initial) : value(std::move(initial)) {}

        T value;
        std::mutex mutex;
        std::vector<Callback> observers;
    };

    struct Ownership {
        std::mutex mutex;
        std::vector<Subscription> subscriptions;
    };

    std::shared_ptr<State> state_{};
    std::shared_ptr<Ownership> ownership_{};
};

template <typename T, typename Fn>
auto signal_map(Signal<T>& input, Fn&& fn) {
    using Result = std::invoke_result_t<Fn, const T&>;
    Signal<Result> output(fn(input.value()));

    auto update = [output, func = std::forward<Fn>(fn)](const T& value) mutable {
        output.set(func(value));
    };

    output.keep_alive(input.subscribe(std::move(update)));
    return output;
}

template <typename A, typename B, typename Fn>
auto signal_combine(Signal<A>& left, Signal<B>& right, Fn&& fn) {
    using Result = std::invoke_result_t<Fn, const A&, const B&>;
    Signal<Result> output(fn(left.value(), right.value()));

    auto update = [output, &left, &right, func = std::forward<Fn>(fn)](const auto&) mutable {
        output.set(func(left.value(), right.value()));
    };

    output.keep_alive(left.subscribe(update));
    output.keep_alive(right.subscribe(update));
    return output;
}

}  // namespace carl
