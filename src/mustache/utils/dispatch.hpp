#pragma once

#include <cstdint>
#include <vector>
#include <thread>
#include <memory>
#include <future>

#include <mustache/utils/uncopiable.hpp>
#include <mustache/utils/index_like.hpp>

namespace mustache {

    // 0 - thread is not controlled by Dispatcher.
    // 1..N for Dispatcher threads, where N - number of threads.
    struct ThreadId : public IndexLike<uint32_t, ThreadId>{};

    using Job = std::function<void(ThreadId)>;
    using QueueId = uint32_t;
    class Dispatcher;

    enum CommonQueuePriority : int32_t {
        kDefault = 0,
        kHigh = 100,
        kLow = -100,
        kBackground = -1000
    };

    class Queue : public Uncopiable {
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

    class Dispatcher : public Uncopiable {
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

        void addParallelTask(std::function<void()>&& job) {
            addParallelTask([no_arg_job_job = std::move(job)](ThreadId) {
                no_arg_job_job();
            });
        }

        Queue createQueue(const std::string& name, int32_t priority = CommonQueuePriority::kDefault);

        void async(uint32_t queue_id, Job&& job);
        void async(uint32_t queue_id, std::function<void()>&& job) {
            async(queue_id, [no_arg_job_job = std::move(job)](ThreadId) {
                no_arg_job_job();
            });
        }

        void waitQueue(QueueId queue_id) const noexcept;

        void setSingleThreadMode(bool on) noexcept;
    private:
        void addJob(Job&& job);
        struct Data;
        std::unique_ptr<Data> data_;
    };

}
