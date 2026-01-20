#pragma once

#include <utility>

#include "carl/scheduler.h"
#include "carl/signal.h"
#include "carl/stream.h"

namespace carl {

class ReactiveContext {
public:
    explicit ReactiveContext(Scheduler& scheduler) : scheduler_(scheduler) {}

    Scheduler& scheduler() const {
        return scheduler_;
    }

    template <typename T>
    Signal<T> signal(T initial) const {
        return Signal<T>(std::move(initial));
    }

    template <typename T>
    Stream<T> stream() const {
        return Stream<T>();
    }

    template <typename T, typename Fn>
    auto signal_map(Signal<T>& input, Fn&& fn) const {
        return carl::signal_map(scheduler_, input, std::forward<Fn>(fn));
    }

    template <typename A, typename B, typename Fn>
    auto signal_combine(Signal<A>& left, Signal<B>& right, Fn&& fn) const {
        return carl::signal_combine(scheduler_, left, right, std::forward<Fn>(fn));
    }

    template <typename T, typename Fn>
    auto stream_map(Stream<T>& input, Fn&& fn) const {
        return carl::stream_map(scheduler_, input, std::forward<Fn>(fn));
    }

    template <typename T, typename Pred>
    auto stream_filter(Stream<T>& input, Pred&& pred) const {
        return carl::stream_filter(scheduler_, input, std::forward<Pred>(pred));
    }

    template <typename T, typename Acc, typename Fn>
    auto stream_fold(Stream<T>& input, Acc seed, Fn&& fn) const {
        return carl::stream_fold(scheduler_, input, std::move(seed), std::forward<Fn>(fn));
    }

private:
    Scheduler& scheduler_;
};

}  // namespace carl
