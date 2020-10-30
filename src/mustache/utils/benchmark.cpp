#include "benchmark.hpp"
#include <algorithm>
#include <cmath>
#include <mustache/utils/logger.hpp>

using namespace mustache;

void Benchmark::show() {
    if(times_.empty()) {
        return;
    }
    if(times_.size() < 2) {
        std::cout<<"Time: " << times_.front() << "ms" << std::endl;
        return;
    }
    const auto sum = std::accumulate(times_.begin(), times_.end(), 0.0);
    std::sort(times_.begin(), times_.end());
    auto med = times_[times_.size() / 2];
    if(times_.size() % 2 == 0) {
        med = (med + times_[times_.size() / 2 - 1]) * 0.5;
    }
    const auto avr = sum / times_.size();
    double variance = 0.0;
    for (auto x : times_) {
        variance += (avr - x) * (avr - x) / times_.size();
    }

    Logger{}.hideContext().info("Avr: %fms, med: %fms, min: %fms,"
        " max: %fms, variance: %fms, sigma: %fms\n",
        avr, med, times_.front(), times_.back(), variance, sqrt(variance));

}

void Benchmark::reset() {
    times_.clear();
}
