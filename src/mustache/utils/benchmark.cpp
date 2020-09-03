#include "benchmark.hpp"
#include <algorithm>

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
    std::cout<<"Avr: " <<  sum / times_.size() << "ms, med: " << med << "ms, min: "
             << times_.front() << "ms, max: "<< times_.back()<<"ms"<< std::endl;
}

void Benchmark::reset() {
    times_.clear();
}
