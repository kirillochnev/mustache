#include "timer.hpp"

#ifndef __clang__
// including chrono with new version of clang leads to compilation error
#include <chrono>
#else
#include <time.h>
#endif

using namespace mustache;
namespace {
#ifndef __clang__
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;

    double get_seconds(const TimePoint& begin, const TimePoint& end) {
        constexpr double Nanoseconds2SecondConvertK = 0.000000001;
        const auto microseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();
        return static_cast<double>(microseconds) * Nanoseconds2SecondConvertK;
    }

    TimePoint now() noexcept {
        return Clock::now();
    }
#else
    using TimePoint = timespec;
    TimePoint now() noexcept {
        TimePoint result {0, 0};
        clock_gettime(CLOCK_MONOTONIC, &result);
        return result;
    }
    double get_seconds(TimePoint begin, TimePoint end) noexcept {
        constexpr double nanoseconds_to_seconds = 1.0 / 1000000000.0;
        auto delta = static_cast<double>(end.tv_sec - begin.tv_sec);
        delta += static_cast<double>(end.tv_nsec - begin.tv_nsec) * nanoseconds_to_seconds;
        return delta;
    }
#endif
}

struct Timer::Data {
    TimePoint begin = now();
    TimePoint pause_begin;
    double pause_time = 0.0;
    mutable double elapsed = 0.0;
    TimerStatus status = TimerStatus::Active;
};

//-------------------------------------------------------------------------------------------------
Timer::Timer():
        data_{} {
}
//-------------------------------------------------------------------------------------------------
Timer::~Timer() = default;
//-------------------------------------------------------------------------------------------------
double Timer::elapsed() const{
    if(data_->status == TimerStatus::Active){
        data_->elapsed = get_seconds(data_->begin, now()) - data_->pause_time;
    }
    return data_->elapsed;
}
//-------------------------------------------------------------------------------------------------
TimerStatus Timer::status() const{
    return data_->status;
}
//-------------------------------------------------------------------------------------------------

void Timer::Timer::reset(){
    data_->elapsed = 0.0;
    data_->pause_time = 0.0;
    data_->begin = now();
    data_->status = TimerStatus::Active;
}
//-------------------------------------------------------------------------------------------------
void Timer::pause(){
    if(data_->status != TimerStatus::Active){
        return;
    }
    data_->elapsed = get_seconds(data_->begin, now());
    data_->status = TimerStatus::Paused;
    data_->pause_begin = now();
}
//-------------------------------------------------------------------------------------------------
void Timer::resume(){
    data_->status = TimerStatus::Active;
    data_->pause_time += get_seconds(data_->pause_begin, now());
}

//-------------------------------------------------------------------------------------------------
