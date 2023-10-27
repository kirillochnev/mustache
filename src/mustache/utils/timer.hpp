#pragma once

#include <mustache/utils/dll_export.h>
#include <mustache/utils/fast_private_impl.hpp>

namespace mustache {

    enum class TimerStatus {
        Paused = 0,
        Active = 1,
    };

    class MUSTACHE_EXPORT Timer {
    public:
        Timer();
        ~Timer();

        void reset();

        void pause();

        void resume();

        double elapsed() const;

        TimerStatus status() const;

    protected:
        struct Data;
        FastPimpl<Data, 64, 8> data_;
    };

}
