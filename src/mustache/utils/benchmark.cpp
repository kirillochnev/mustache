#include "benchmark.hpp"

#include <mustache/utils/logger.hpp>

#include <cmath>
#include <numeric>
#include <algorithm>

using namespace mustache;

void Benchmark::show() {
    if(times_.empty()) {
        return;
    }
    if(times_.size() < 2) {
        Logger{}.hideContext().info("Time: %fms", static_cast<float>(times_.front()));
        return;
    }
    const auto sum = std::accumulate(times_.begin(), times_.end(), 0.0);
    std::sort(times_.begin(), times_.end());
    auto med = times_[times_.size() / 2];
    if(times_.size() % 2 == 0) {
        med = (med + times_[times_.size() / 2 - 1]) * 0.5;
    }
    const auto avr = sum / static_cast<double>(times_.size());
    double variance = 0.0;
    for (auto x : times_) {
        variance += (avr - x) * (avr - x) / static_cast<double>(times_.size());
    }

    Logger{}.hideContext().info("Call count: %d, Avr: %fms, med: %fms, min: %fms,"
        " max: %fms, variance: %fms, sigma: %fms\n", static_cast<uint32_t>(times_.size()),
        avr, med, times_.front(), times_.back(), variance, sqrt(variance));

}

void Benchmark::reset() {
    times_.clear();
}
