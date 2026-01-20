#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <type_traits>
#include <utility>
#include <vector>

#include "carl/scheduler.h"
#include "carl/signal.h"
#include "carl/subscription.h"
#include "carl/task.h"

namespace carl {

template <typename T>
class Stream {
public:
    using Callback = std::function<void(const T&)>;

    Stream() : state_(std::make_shared<State>()), ownership_(std::make_shared<Ownership>()) {}

    void emit(T value) {
        std::vector<Callback> callbacks;
        {
            std::lock_guard<std::mutex> lock(state_->mutex);
            callbacks = state_->observers;
        }

        for (const auto& callback : callbacks) {
            if (callback) {
                callback(value);
            }
        }
    }

    void emit(Scheduler& scheduler, T value) {
        std::vector<Callback> callbacks;
        {
            std::lock_guard<std::mutex> lock(state_->mutex);
            callbacks = state_->observers;
        }

        scheduler.spawn(dispatch_callbacks(std::move(callbacks), std::move(value)));
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
    static Task dispatch_callbacks(std::vector<Callback> callbacks, T value) {
        for (const auto& callback : callbacks) {
            if (callback) {
                callback(value);
            }
        }
        co_return;
    }

    struct State {
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
auto stream_map(Stream<T>& input, Fn&& fn) {
    using Result = std::invoke_result_t<Fn, const T&>;
    Stream<Result> output;

    auto forward = [output, func = std::forward<Fn>(fn)](const T& value) mutable {
        output.emit(func(value));
    };

    output.keep_alive(input.subscribe(std::move(forward)));
    return output;
}

template <typename T, typename Fn>
auto stream_map(Scheduler& scheduler, Stream<T>& input, Fn&& fn) {
    using Result = std::invoke_result_t<Fn, const T&>;
    Stream<Result> output;

    auto forward = [output, &scheduler, func = std::forward<Fn>(fn)](const T& value) mutable {
        output.emit(scheduler, func(value));
    };

    output.keep_alive(input.subscribe(std::move(forward)));
    return output;
}

template <typename T, typename Pred>
auto stream_filter(Stream<T>& input, Pred&& pred) {
    Stream<T> output;

    auto forward = [output, predicate = std::forward<Pred>(pred)](const T& value) mutable {
        if (predicate(value)) {
            output.emit(value);
        }
    };

    output.keep_alive(input.subscribe(std::move(forward)));
    return output;
}

template <typename T, typename Pred>
auto stream_filter(Scheduler& scheduler, Stream<T>& input, Pred&& pred) {
    Stream<T> output;

    auto forward = [output, &scheduler, predicate = std::forward<Pred>(pred)](const T& value) mutable {
        if (predicate(value)) {
            output.emit(scheduler, value);
        }
    };

    output.keep_alive(input.subscribe(std::move(forward)));
    return output;
}

template <typename T, typename Acc, typename Fn>
auto stream_fold(Stream<T>& input, Acc seed, Fn&& fn) {
    Signal<Acc> output(seed);

    auto forward = [output, func = std::forward<Fn>(fn)](const T& value) mutable {
        output.set(func(output.value(), value));
    };

    output.keep_alive(input.subscribe(std::move(forward)));
    return output;
}

template <typename T, typename Acc, typename Fn>
auto stream_fold(Scheduler& scheduler, Stream<T>& input, Acc seed, Fn&& fn) {
    Signal<Acc> output(seed);

    auto forward = [output, &scheduler, func = std::forward<Fn>(fn)](const T& value) mutable {
        output.set(scheduler, func(output.value(), value));
    };

    output.keep_alive(input.subscribe(std::move(forward)));
    return output;
}

}  // namespace carl
