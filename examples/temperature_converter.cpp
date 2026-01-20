#include <cmath>
#include <iostream>
#include <string>

#include "carl/actor.h"
#include "carl/reactive_context.h"
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

    carl::ReactiveContext context(scheduler);
    carl::Signal<double> celsius(0.0);
    carl::Signal<double> room_temp(22.0);

    auto fahrenheit = context.signal_map(celsius, [](double c) {
        return (c * 9.0 / 5.0) + 32.0;
    });
    auto rounded_f = context.signal_map(fahrenheit, [](double f) {
        return std::round(f);
    });
    auto kelvin = context.signal_map(celsius, [](double c) {
        return c + 273.15;
    });
    auto status = context.signal_map(celsius, [](double c) {
        if (c <= 0.0) {
            return std::string("freezing");
        }
        if (c >= 100.0) {
            return std::string("boiling");
        }
        return std::string("liquid");
    });
    auto delta_from_room = context.signal_combine(celsius, room_temp, [](double c, double room) {
        return c - room;
    });

    LoggerActor logger(scheduler);
    auto log_subscription = logger.subscribe(fahrenheit, [](double value) {
        std::cout << "Fahrenheit: " << value << "\n";
    });
    auto rounded_subscription = logger.subscribe(rounded_f, [](double value) {
        std::cout << "Rounded F: " << value << "\n";
    });
    auto kelvin_subscription = logger.subscribe(kelvin, [](double value) {
        std::cout << "Kelvin: " << value << "\n";
    });
    auto status_subscription = logger.subscribe(status, [](const std::string& value) {
        std::cout << "Status: " << value << "\n";
    });
    auto delta_subscription = logger.subscribe(delta_from_room, [](double value) {
        std::cout << "Delta from room: " << value << "\n";
    });

    scheduler.spawn(logger.run());

    logger.set(celsius, 25.0);
    logger.set(room_temp, 20.0);
    logger.set(celsius, 30.0);
    logger.post([&logger]() { logger.stop(); });

    scheduler.run();
    return 0;
}
