#pragma once

#include <atomic>
#include <functional>
#include <utility>

#include "carl/mailbox.h"
#include "carl/scheduler.h"
#include "carl/signal.h"
#include "carl/stream.h"
#include "carl/subscription.h"
#include "carl/task.h"

namespace carl {

class Actor {
public:
    using Message = std::function<void()>;

    explicit Actor(Scheduler& scheduler) : scheduler_(scheduler) {}
    virtual ~Actor() = default;

    void post(Message message) {
        mailbox_.push(std::move(message));
    }

    void stop() {
        running_.store(false);
        post([] {});
    }

    Task run() {
        on_start();
        while (running_.load()) {
            Message message;
            if (mailbox_.try_pop(message)) {
                message();
            } else {
                co_await scheduler_.yield();
            }
        }
        on_stop();
    }

    template <typename T, typename Fn>
    Subscription subscribe(Stream<T>& stream, Fn&& handler) {
        Actor* self = this;
        auto callback = [self, func = std::forward<Fn>(handler)](const T& value) mutable {
            self->post([func, value]() mutable { func(value); });
        };
        return stream.subscribe(std::move(callback));
    }

    template <typename T, typename Fn>
    Subscription subscribe(Signal<T>& signal, Fn&& handler) {
        Actor* self = this;
        auto callback = [self, func = std::forward<Fn>(handler)](const T& value) mutable {
            self->post([func, value]() mutable { func(value); });
        };
        return signal.subscribe(std::move(callback));
    }

    template <typename T>
    void set(Signal<T>& signal, T value) {
        post([this, &signal, payload = std::move(value)]() mutable {
            signal.set(scheduler_, std::move(payload));
        });
    }

    template <typename T>
    void emit(Stream<T>& stream, T value) {
        post([this, &stream, payload = std::move(value)]() mutable {
            stream.emit(scheduler_, std::move(payload));
        });
    }

protected:
    virtual void on_start() {}
    virtual void on_stop() {}

private:
    Scheduler& scheduler_;
    Mailbox<Message> mailbox_{};
    std::atomic<bool> running_{true};
};

}  // namespace carl
