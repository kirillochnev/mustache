#pragma once

#include <mustache/utils/uncopiable.hpp>
#include <mustache/utils/index_like.hpp>

#include <cstdint>
#include <vector>
#include <thread>
#include <memory>
#include <future>

namespace mustache {

    struct MUSTACHE_EXPORT ThreadId : public IndexLike<uint32_t, ThreadId>{};

    using Job = std::function<void(ThreadId)>;
    using QueueId = uint32_t;
    class Dispatcher;

    enum CommonQueuePriority : int32_t {
        kDefault = 0,
        kHigh = 100,
        kLow = -100,
        kBackground = -1000
    };

    class MUSTACHE_EXPORT Queue : public Uncopiable {
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

    struct ParallelTaskId : public IndexLike<uint32_t , ParallelTaskId> {};
    struct ParallelTaskItemIndexInTask : public IndexLike<uint32_t , ParallelTaskItemIndexInTask> {};
    struct ParallelTaskGlobalItemIndex : public IndexLike<uint32_t , ParallelTaskGlobalItemIndex> {};

    class MUSTACHE_EXPORT Dispatcher : public Uncopiable {
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


        void initThreads();

        void clear() noexcept;

        // blocks calling thread until job queue is empty
        void waitForParallelFinish() const noexcept;

        template<typename _F>
        void parallelFor(_F&& function, size_t begin, size_t end, uint32_t task_count = 0u) {
            const size_t size = end - begin;
            if (task_count < 1u) {
                task_count = size < threadCount() ? static_cast<uint32_t>(size) : threadCount();
            }
            const size_t ept = size / task_count;
            const size_t tasks_with_extra_item = size - task_count * ept;

            size_t task_begin = begin;
            for (uint32_t task = 0; task < task_count; ++task) {
                const auto task_size = task < tasks_with_extra_item ? ept + 1 : ept;
                const auto task_id = ParallelTaskId::make(task);
                addParallelTask([task_id, &function, task_end = task_begin + task_size, task_begin]{
                    for (size_t i = task_begin; i < task_end; ++i) {
                        invoke(function, i, task_id);
                    }
                });

                task_begin += task_size;
            }
            waitForParallelFinish();
        }

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

        // 0 - thread is not controlled by Dispatcher.
        // 1..threadCount for Dispatcher threads.
        [[nodiscard]] ThreadId currentThreadId() const noexcept;
    private:
        void addJob(Job&& job);
        struct Data;
        std::unique_ptr<Data> data_;
    };

}
