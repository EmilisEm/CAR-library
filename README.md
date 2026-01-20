# CARL: Actor–Reactor Reactive Library (PoC)

CARL is a proof-of-concept reactive library for C++23 inspired by the Actor–Reactor model. It separates:

- **Actors**: imperative, stateful, side-effect handlers (mailbox + message loop).
- **Reactors**: functional reactive composition (Signals/Streams + operators).

This PoC focuses on clarity and architecture rather than production optimizations.

## Key Concepts

- **Signal<T>**: a stateful value that always has a current value.
- **Stream<T>**: a sequence of discrete events (no stored value).
- **Actor**: processes messages sequentially, handles side effects.
- **ReactiveContext**: wraps a Scheduler and provides operator helpers.
- **Scheduler**: thread-pool coroutine scheduler for concurrent propagation.

## Quick Example

```cpp
carl::Scheduler scheduler;
carl::ReactiveContext context(scheduler);

carl::Signal<double> celsius(0.0);
auto fahrenheit = context.signal_map(celsius, [](double c) {
    return (c * 9.0 / 5.0) + 32.0;
});

class Logger : public carl::Actor {
public:
    explicit Logger(carl::Scheduler& s) : carl::Actor(s) {}
};

Logger logger(scheduler);
auto sub = logger.subscribe(fahrenheit, [](double f) {
    std::cout << "F: " << f << "\n";
});

scheduler.spawn(logger.run());
logger.set(celsius, 25.0);
logger.post([&]() { logger.stop(); });

scheduler.run();
```

## Build & Run

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build

./build/example_temperature
./build/example_stream_fanout
./build/example_long_running
```

## Architecture Notes

- Reactive propagation is **per-node parallel**: each Signal/Stream update is dispatched as a task on the Scheduler thread pool.
- Actor side effects remain serialized per actor mailbox.
- Signals and Streams keep subscriptions alive internally for derived nodes.

## Examples

- `examples/temperature_converter.cpp`: multiple derived signals (F/K/status/delta).
- `examples/stream_fanout.cpp`: fan-out streams, chained operators, running sum.
- `examples/long_running.cpp`: coroutine-driven progress stream with derived streams.

## Tests

- `tests/carl_tests.cpp`: multi-node propagation and actor loop tests.

## Status

This is an educational proof-of-concept. It is not intended for production use.

## Reference

Inspired by *Tackling the Awkward Squad for Reactive Programming: The Actor–Reactor Model*.

