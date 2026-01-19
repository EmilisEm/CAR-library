#include <iostream>
#include <string>

#include "carl/actor.h"
#include "carl/scheduler.h"
#include "carl/stream.h"

namespace {

class PrinterActor : public carl::Actor {
public:
    PrinterActor(carl::Scheduler& scheduler, std::string label)
        : carl::Actor(scheduler), label_(std::move(label)) {}

    void print(int value) {
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

    carl::Stream<int> numbers;
    auto doubled = carl::stream_map(numbers, [](int value) { return value * 2; });
    auto even = carl::stream_filter(numbers, [](int value) { return value % 2 == 0; });

    ProducerActor producer(scheduler);
    PrinterActor doubled_printer(scheduler, "double");
    PrinterActor even_printer(scheduler, "even");

    auto sub1 = doubled_printer.subscribe(doubled, [&doubled_printer](int value) {
        doubled_printer.print(value);
    });
    auto sub2 = even_printer.subscribe(even, [&even_printer](int value) {
        even_printer.print(value);
    });

    scheduler.spawn(producer.run());
    scheduler.spawn(doubled_printer.run());
    scheduler.spawn(even_printer.run());

    for (int i = 1; i <= 5; ++i) {
        producer.emit(numbers, i);
    }

    producer.post([&producer]() { producer.stop(); });
    doubled_printer.post([&doubled_printer]() { doubled_printer.stop(); });
    even_printer.post([&even_printer]() { even_printer.stop(); });

    scheduler.run();
    return 0;
}
