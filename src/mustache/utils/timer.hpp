#pragma once

#include <mustache/utils/dll_export.h>

#include <chrono>

namespace mustache {

    enum class TimerStatus {
        Paused = 0,
        Active = 1,
    };

    class MUSTACHE_EXPORT Timer {
    public:
        void reset();

        void pause();

        void resume();

        double elapsed() const;

        TimerStatus status() const;

    protected:
        using Clock = std::chrono::high_resolution_clock;
        using TimePoint = Clock::time_point;

        TimePoint begin_ = Clock::now();
        TimePoint pause_begin_;
        double pause_time_ = 0.0;
        mutable double elapsed_ = 0.0;
        TimerStatus status_ = TimerStatus::Active;
    };

}