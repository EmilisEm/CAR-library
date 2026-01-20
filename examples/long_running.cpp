#include <iostream>

#include "carl/actor.h"
#include "carl/scheduler.h"
#include "carl/stream.h"
#include "carl/task.h"

namespace {

class ProgressActor : public carl::Actor {
public:
    explicit ProgressActor(carl::Scheduler& scheduler) : carl::Actor(scheduler) {}
};

carl::Task compute_progress(carl::Scheduler& scheduler, carl::Stream<int>& progress) {
    for (int step = 0; step <= 4; ++step) {
        progress.emit(scheduler, step);
        co_await scheduler.yield();
    }
}

}  // namespace

int main() {
    carl::Scheduler scheduler;
    carl::Stream<int> progress;

    ProgressActor logger(scheduler);
    auto sub = logger.subscribe(progress, [&logger](int step) {
        std::cout << "progress: " << step << "\n";
        if (step == 4) {
            logger.stop();
        }
    });

    scheduler.spawn(logger.run());
    scheduler.spawn(compute_progress(scheduler, progress));
    scheduler.run();

    return 0;
}
