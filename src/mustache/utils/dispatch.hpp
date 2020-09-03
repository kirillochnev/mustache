#pragma once

#include <cstdint>
#include <vector>
#include <thread>
#include <memory>
#include <future>

namespace mustache {
    using Job = std::function<void()>;
    using QueueId = uint32_t;
    class Dispatcher;

    enum CommonQueuePriority : int32_t {
        kDefault = 0,
        kHigh = 100,
        kLow = -100,
        kBackground = -1000
    };

    class Queue {
    public:
        bool valid() const noexcept {
            return dispatcher_ != nullptr && id_ != static_cast<QueueId>(-1);
        }
        void async(Job&& job);
        void wait() const;
    private:
        friend Dispatcher;
        Dispatcher* dispatcher_{nullptr};
        QueueId id_{static_cast<QueueId>(-1)};
    };

    class Dispatcher {
    public:
        uint32_t threadCount() const noexcept;
        static uint32_t maxThreadCount() noexcept;

        // starts threadCount threads, waiting for jobs
        // may throw a std::system_error if a thread could not be started
        explicit Dispatcher(uint32_t thread_count = 0);
        // non-copyable,
        Dispatcher(const Dispatcher&) = delete;
        Dispatcher& operator=(const Dispatcher&) = delete;

        // but movable
        Dispatcher(Dispatcher&&);
        Dispatcher& operator=(Dispatcher&&);

        ~Dispatcher();

        void clear() noexcept;

        // blocks calling thread until job queue is empty
        void waitForParallelFinish() const noexcept;

        void addParallelTask(Job&& job) {
            addJob(std::move(job));
        }

        Queue createQueue(const std::string& name, int32_t priority = CommonQueuePriority::kDefault);

        void async(uint32_t QueueId, Job&& job);
        void waitQueue(QueueId queue_id) const noexcept;

        void setSingleThreadMode(bool on) noexcept;
    private:
        void addJob(Job&& job);
        struct Data;
        std::unique_ptr<Data> data_;
    };

}
