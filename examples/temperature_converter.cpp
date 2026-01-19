#include <iostream>

#include "carl/actor.h"
#include "carl/scheduler.h"
#include "carl/signal.h"

namespace {

class LoggerActor : public carl::Actor {
public:
    explicit LoggerActor(carl::Scheduler& scheduler) : carl::Actor(scheduler) {}
};

}  // namespace

int main() {
    carl::Scheduler scheduler;

    carl::Signal<double> celsius(0.0);
    auto fahrenheit = carl::signal_map(celsius, [](double c) {
        return (c * 9.0 / 5.0) + 32.0;
    });

    LoggerActor logger(scheduler);
    auto log_subscription = logger.subscribe(fahrenheit, [](double value) {
        std::cout << "Fahrenheit: " << value << "\n";
    });

    scheduler.spawn(logger.run());

    logger.set(celsius, 25.0);
    logger.set(celsius, 30.0);
    logger.post([&logger]() { logger.stop(); });

    scheduler.run();
    return 0;
}
