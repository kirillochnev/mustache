#include "dispatch.hpp"
#include <queue>
#include <map>
#include <set>

#ifdef __EMSCRIPTEN__
#include<emscripten/threading.h>
#define NUMBER_OF_CORES emscripten_num_logical_cores()
#else
#define NUMBER_OF_CORES std::thread::hardware_concurrency()
#endif

using namespace mustache;

namespace {
    enum class JobState : uint8_t {
        kParallelQueue = 0u,
        kUnlocked,
        kLocked
    };

    struct JobQueue {
        std::queue<Job> jobs;
        JobState state{JobState::kParallelQueue};

        bool isEmpty() const noexcept {
            return jobs.empty();
        }
        void onTaskBegin() noexcept {
            if(state == JobState::kUnlocked) {
                state = JobState ::kLocked;
            }
        }
        void onTaskEnd() noexcept {
            if(state == JobState::kLocked) {
                state = JobState ::kUnlocked;
            }
        }
        bool isLocked() const noexcept {
            return state == JobState::kLocked;
        }
        /// TODO: rename
        bool isOk() const noexcept {
            return !isLocked() && !jobs.empty();
        }

        Job&& front() noexcept {
            return std::move(jobs.front());
        }

        void pop() {
            jobs.pop();
        }

        void push(Job&& job) {
            jobs.emplace(std::move(job));
        }

        void clear() {
            while (!jobs.empty()) {
                jobs.pop();
            }
        }
    };

    thread_local ThreadId g_thread_id;
}

struct Dispatcher::Data {
    JobQueue parallel_jobs;
    mutable std::mutex mutex;
    std::condition_variable jobs_available;
    std::vector<std::thread> threads;
    std::set<std::thread::id> thread_ids;
    uint32_t threads_waiting{0u};

    struct {
        std::vector<std::unique_ptr<JobQueue> > array;
        std::multimap<int32_t, QueueId> by_priority;
        std::map<std::string, QueueId> by_name;
    } extra;

    bool terminate {false};
    bool single_thread_mode{false};

    JobQueue* findQueue() {
        if(parallel_jobs.isOk()) {
            return &parallel_jobs;
        }
        for(auto& pair : extra.by_priority) {
            JobQueue* ptr = extra.array[pair.second].get();
            if(ptr->isOk()) {
                return ptr;
            }
        }
        return nullptr;
    }

    [[nodiscard]] ThreadId currentThreadId() noexcept {
        auto thread_id = g_thread_id;
        if (thread_id.isNull() || thread_ids.count(std::this_thread::get_id()) < 1) {
            thread_id = ThreadId::make(0);
        }
        return thread_id;
    }

#define DOUBLE_LOCK 1

    void threadTask(ThreadId thread_id) noexcept {
        g_thread_id = thread_id;
        JobQueue* queue = nullptr;
        while (!terminate) {
            std::unique_lock<std::mutex> lock{ mutex };
#if !DOUBLE_LOCK
            if(queue) {
                queue->onTaskEnd();
            }
#endif
            queue = findQueue();
            while (!terminate && !queue) {
                ++threads_waiting;
                jobs_available.wait(lock);
                --threads_waiting;
                queue = findQueue();
            }
            if (terminate) {
                break;
            }

            auto job = std::move(queue->front());
            queue->pop();
            queue->onTaskBegin();
            lock.unlock();
            job(thread_id);
#if DOUBLE_LOCK
            lock.lock();
            queue->onTaskEnd();
#endif
        }
    }

    void wait(JobQueue& queue) {
        while (!terminate) {
            std::unique_lock<std::mutex> lock{ mutex };
            if(queue.isEmpty()) {
                break;
            }
            auto job = std::move(queue.front());

            queue.pop();
            queue.onTaskBegin();
            lock.unlock();
            job(currentThreadId());
            lock.lock();
            queue.onTaskEnd();
        }
        if(queue.state == JobState::kParallelQueue) {
            const auto num_threads = threads.size();
            while (threads_waiting != num_threads) {
                std::this_thread::yield();
            }
        } else {
            while (queue.isLocked()) {
                std::this_thread::yield();
            }
        }
    }
};

Dispatcher::Dispatcher(uint32_t thread_count):
        data_{new Data} {

    if (thread_count == 0) {
        thread_count = maxThreadCount() - 1; // one core for main thread
    }

    data_->threads.reserve(thread_count);
    for(uint32_t i = 0; i < thread_count; ++i) {
        auto& thread = data_->threads.emplace_back([this, i]() noexcept {
            data_->threadTask(ThreadId::make(i + 1));
        });
        data_->thread_ids.insert(thread.get_id());
    }
}

Dispatcher::Dispatcher(Dispatcher&&) = default;
Dispatcher& Dispatcher::operator=(Dispatcher&&) = default;

Dispatcher::~Dispatcher() {
    if(!data_) {
        return;
    }
    data_->terminate = true;
    clear();
    data_->jobs_available.notify_all();
    for (auto& thread : data_->threads)  {
        if (thread.joinable())
            thread.join();
    }
}

void Dispatcher::clear() noexcept {
    // TODO: ?
    std::lock_guard<std::mutex> lock { data_->mutex };
    data_->parallel_jobs.clear();
}

void Dispatcher::waitForParallelFinish() const noexcept {
    data_->wait(data_->parallel_jobs);
}

void Dispatcher::addJob(Job&& job) {
    if(data_->single_thread_mode) {
        job(ThreadId::make(0));
        return;
    }
    {
        std::unique_lock<std::mutex> lock{ data_->mutex };
        data_->parallel_jobs.push(std::move(job));
    }
    data_->jobs_available.notify_one();
}

Queue Dispatcher::createQueue(const std::string& name, int32_t priority) {
    std::lock_guard<std::mutex> lock { data_->mutex };
    const QueueId index = static_cast<QueueId>(data_->extra.array.size());
    auto& info = data_->extra.array.emplace_back();
    info.reset(new JobQueue);
    info->state = JobState::kUnlocked;
    data_->extra.by_name.emplace(name, index);
    data_->extra.by_priority.emplace(priority, index);
    Queue result;
    result.id_ = index;
    result.dispatcher_ = this;
    return result;
}

void Dispatcher::async(QueueId queue_id, Job &&job) {
    {
        std::lock_guard<std::mutex> lock{data_->mutex};
        data_->extra.array[queue_id]->jobs.emplace(std::move(job));
    }
    data_->jobs_available.notify_one();
}

void Dispatcher::waitQueue(QueueId queue_id) const noexcept {
    data_->wait(*data_->extra.array[queue_id]);
}

uint32_t Dispatcher::maxThreadCount() noexcept {
    return NUMBER_OF_CORES;
}

void Dispatcher::setSingleThreadMode(bool on) noexcept {
    std::lock_guard<std::mutex> lock{data_->mutex};
    data_->single_thread_mode = on;
}

uint32_t mustache::Dispatcher::threadCount() const noexcept {
    return static_cast<uint32_t>(data_->threads.size());
}

ThreadId mustache::Dispatcher::currentThreadId() const noexcept {
    return data_->currentThreadId();
}

void Queue::async(Job&& job) {
    if(!valid()) {
        throw std::runtime_error("Invalid queue");
    }
    dispatcher_->async(id_, std::move(job));
}

void Queue::wait() const {
    if(!valid()) {
        throw std::runtime_error("Invalid queue");
    }
    dispatcher_->waitQueue(id_);
}
