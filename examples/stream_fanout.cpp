#include <iostream>
#include <string>

#include "carl/actor.h"
#include "carl/reactive_context.h"
#include "carl/scheduler.h"
#include "carl/signal.h"
#include "carl/stream.h"

namespace {

class PrinterActor : public carl::Actor {
public:
    PrinterActor(carl::Scheduler& scheduler, std::string label)
        : carl::Actor(scheduler), label_(std::move(label)) {}

    void print(int value) {
        std::cout << label_ << ": " << value << "\n";
    }

    void print_text(const std::string& value) {
        std::cout << label_ << ": " << value << "\n";
    }

private:
    std::string label_;
};

class ProducerActor : public carl::Actor {
public:
    explicit ProducerActor(carl::Scheduler& scheduler) : carl::Actor(scheduler) {}
};

}  // namespace

int main() {
    carl::Scheduler scheduler;

    carl::ReactiveContext context(scheduler);
    carl::Stream<int> numbers;
    auto doubled = context.stream_map(numbers, [](int value) { return value * 2; });
    auto squared = context.stream_map(doubled, [](int value) { return value * value; });
    auto even = context.stream_filter(numbers, [](int value) { return value % 2 == 0; });
    auto labeled = context.stream_map(numbers, [](int value) {
        return std::string("n=") + std::to_string(value);
    });
    auto running_sum = context.stream_fold(numbers, 0, [](int acc, int value) {
        return acc + value;
    });

    ProducerActor producer(scheduler);
    PrinterActor doubled_printer(scheduler, "double");
    PrinterActor squared_printer(scheduler, "squared");
    PrinterActor even_printer(scheduler, "even");
    PrinterActor label_printer(scheduler, "label");
    PrinterActor sum_printer(scheduler, "sum");

    auto sub1 = doubled_printer.subscribe(doubled, [&doubled_printer](int value) {
        doubled_printer.print(value);
    });
    auto sub2 = squared_printer.subscribe(squared, [&squared_printer](int value) {
        squared_printer.print(value);
    });
    auto sub3 = even_printer.subscribe(even, [&even_printer](int value) {
        even_printer.print(value);
    });
    auto sub4 = label_printer.subscribe(labeled, [&label_printer](const std::string& value) {
        label_printer.print_text(value);
    });
    auto sub5 = sum_printer.subscribe(running_sum, [&sum_printer](int value) {
        sum_printer.print(value);
    });

    scheduler.spawn(producer.run());
    scheduler.spawn(doubled_printer.run());
    scheduler.spawn(squared_printer.run());
    scheduler.spawn(even_printer.run());
    scheduler.spawn(label_printer.run());
    scheduler.spawn(sum_printer.run());

    for (int i = 1; i <= 5; ++i) {
        producer.emit(numbers, i);
    }

    producer.post([&producer]() { producer.stop(); });
    doubled_printer.post([&doubled_printer]() { doubled_printer.stop(); });
    squared_printer.post([&squared_printer]() { squared_printer.stop(); });
    even_printer.post([&even_printer]() { even_printer.stop(); });
    label_printer.post([&label_printer]() { label_printer.stop(); });
    sum_printer.post([&sum_printer]() { sum_printer.stop(); });

    scheduler.run();
    return 0;
}
