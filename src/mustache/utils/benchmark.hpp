#pragma once

#include <mustache/utils/timer.hpp>
#include <mustache/utils/container_deque.hpp>

#include <cstdint>

namespace mustache {
    class MUSTACHE_EXPORT Benchmark {
    public:
        Benchmark() = default;

        void show();
        void reset();

        template <typename T>
        void add(T&& func, uint32_t count = 1) {
            for (uint32_t i = 0; i < count; ++i) {
                timer_.reset();
                func();
                times_.push_back(timer_.elapsed() * 1000);
            }
        }
    private:
        mustache::deque<double > times_;
        Timer timer_;
    };
}

