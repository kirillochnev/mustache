#pragma once

#include <mustache/utils/timer.hpp>

#include <cstdint>
#include <deque>

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
        std::deque<double > times_;
        Timer timer_;
    };
}

