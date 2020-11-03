#include "timer.hpp"

using namespace mustache;
template<typename T>
static double GetSeconds(const T& pDur){
    constexpr double Microseconds2SecondConvertK = 0.000001;
    const auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(pDur).count();
    return static_cast<double>(microseconds) * Microseconds2SecondConvertK;
}


//-------------------------------------------------------------------------------------------------
double Timer::elapsed() const{
    if(status_==TimerStatus::Active){
        elapsed_ = GetSeconds(Clock::now() -begin_) - pause_time_;
    }
    return elapsed_;
}
//-------------------------------------------------------------------------------------------------
TimerStatus Timer::status() const{
    return status_;
}
//-------------------------------------------------------------------------------------------------

void Timer::Timer::reset(){
    elapsed_ = 0.0;
    pause_time_ = 0.0;
    begin_ = Clock::now();
    status_ = TimerStatus::Active;
}
//-------------------------------------------------------------------------------------------------
void Timer::pause(){
    if(status_!=TimerStatus::Active){
        return;
    }
    elapsed_ = GetSeconds(Clock::now() - begin_);
    status_ = TimerStatus::Paused;
    pause_begin_ = Clock::now();
}
//-------------------------------------------------------------------------------------------------
void Timer::resume(){
    status_ = TimerStatus::Active;
    pause_time_+=GetSeconds(Clock::now()- pause_begin_);
}

//-------------------------------------------------------------------------------------------------
