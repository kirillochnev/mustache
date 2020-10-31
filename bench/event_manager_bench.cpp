#include <mustache/ecs/world.hpp>
#include <mustache/utils/benchmark.hpp>


namespace {
    static uint64_t static_uint_value = 0ull;

    void callback(uint64_t i) {
        static_uint_value += i;
    }
}

void bench_events() {
    static constexpr uint64_t kNumEvents = 1000000;
    static constexpr uint64_t kNumIterations = 1000;
    static constexpr uint64_t kSum = (0 + kNumEvents - 1) * kNumEvents * kNumIterations / 2;

    using namespace mustache;
    MemoryManager memory_manager;
    EventManager event_manager{memory_manager};
    auto sub = event_manager.subscribe<uint64_t>(callback);

    Benchmark benchmark;
    benchmark.add([&event_manager]{
        for (uint64_t i = 0; i < kNumEvents; ++i) {
            event_manager.post(i);
        }
    }, kNumIterations);
    benchmark.show();
    benchmark.reset();

    if (kSum != static_uint_value) {
        throw std::runtime_error("invalid sum: " + std::to_string(static_uint_value) + " vs " + std::to_string(kSum));
    }

    static_uint_value = 0ull;

    std::function function = &callback;
    benchmark.add([&function] {
        for (uint64_t i = 0; i < kNumEvents; ++i) {
            function(i);
        }
    }, kNumIterations);
    benchmark.show();
    if (kSum != static_uint_value) {
        throw std::runtime_error("invalid sum: " + std::to_string(static_uint_value) + " vs " + std::to_string(kSum));
    }
}
