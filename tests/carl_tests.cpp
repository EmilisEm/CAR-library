#include <iostream>

#include "carl/actor.h"
#include "carl/reactive_context.h"
#include "carl/scheduler.h"
#include "carl/signal.h"
#include "carl/stream.h"

namespace {

int failures = 0;

#define EXPECT_EQ(left, right)                                                    \
    do {                                                                          \
        auto left_value = (left);                                                 \
        auto right_value = (right);                                               \
        if (left_value != right_value) {                                          \
            std::cerr << "EXPECT_EQ failed: " << #left << " != " << #right      \
                      << " (" << left_value << " vs " << right_value << ")\n"; \
            ++failures;                                                           \
        }                                                                         \
    } while (false)

class TestActor : public carl::Actor {
public:
    explicit TestActor(carl::Scheduler& scheduler) : carl::Actor(scheduler) {}
};

void test_signal_map() {
    carl::Scheduler scheduler(1);
    carl::ReactiveContext context(scheduler);
    carl::Signal<int> source(2);
    auto mapped = context.signal_map(source, [](int value) { return value * 3; });

    EXPECT_EQ(mapped.value(), 6);
    source.set(scheduler, 4);
    scheduler.run();
    EXPECT_EQ(mapped.value(), 12);
}

void test_signal_combine() {
    carl::Scheduler scheduler(1);
    carl::ReactiveContext context(scheduler);
    carl::Signal<int> left(1);
    carl::Signal<int> right(2);
    auto combined = context.signal_combine(left, right, [](int a, int b) { return a + b; });

    EXPECT_EQ(combined.value(), 3);
    left.set(scheduler, 5);
    scheduler.run();
    EXPECT_EQ(combined.value(), 7);
    right.set(scheduler, 4);
    scheduler.run();
    EXPECT_EQ(combined.value(), 9);
}

void test_stream_fold() {
    carl::Scheduler scheduler(1);
    carl::ReactiveContext context(scheduler);
    carl::Stream<int> stream;
    auto sum = context.stream_fold(stream, 0, [](int acc, int value) { return acc + value; });

    stream.emit(scheduler, 1);
    stream.emit(scheduler, 2);
    stream.emit(scheduler, 3);
    scheduler.run();
    EXPECT_EQ(sum.value(), 6);
}

void test_actor_message_loop() {
    carl::Scheduler scheduler;
    carl::Signal<int> signal(0);
    TestActor actor(scheduler);

    scheduler.spawn(actor.run());
    actor.set(signal, 42);
    actor.post([&actor]() { actor.stop(); });
    scheduler.run();

    EXPECT_EQ(signal.value(), 42);
}

}  // namespace

int main() {
    test_signal_map();
    test_signal_combine();
    test_stream_fold();
    test_actor_message_loop();

    if (failures == 0) {
        std::cout << "All tests passed.\n";
        return 0;
    }

    std::cerr << failures << " test(s) failed.\n";
    return 1;
}
