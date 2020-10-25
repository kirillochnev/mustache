#include <gtest/gtest.h>
#include <mustache/utils/dispatch.hpp>

TEST(Dispatcher, currentThreadId) {
    using namespace mustache;
    Dispatcher dispatcher0{5u};
    Dispatcher dispatcher1{5u};
    std::mutex mutex;
    for (uint32_t i = 0; i < 10000; ++i) {
        mutex.lock();
        for (uint32_t thread_index = 0; thread_index < dispatcher0.threadCount(); ++thread_index) {
            dispatcher0.addParallelTask([&mutex, &dispatcher0, &dispatcher1](ThreadId thread_id) {
                std::unique_lock<std::mutex> lock{mutex};
                ASSERT_TRUE(thread_id.toInt() <= dispatcher0.threadCount());
                ASSERT_EQ(thread_id, dispatcher0.currentThreadId());
                ASSERT_EQ(ThreadId::make(0), dispatcher1.currentThreadId());
            });
            dispatcher1.addParallelTask([&mutex, &dispatcher0, &dispatcher1](ThreadId thread_id) {
                std::unique_lock<std::mutex> lock{mutex};
                ASSERT_TRUE(thread_id.toInt() <= dispatcher1.threadCount());
                ASSERT_EQ(thread_id, dispatcher1.currentThreadId());
                ASSERT_EQ(ThreadId::make(0), dispatcher0.currentThreadId());
            });
        }
        ASSERT_EQ(ThreadId::make(0), dispatcher0.currentThreadId());
        ASSERT_EQ(ThreadId::make(0), dispatcher1.currentThreadId());
        mutex.unlock();
        dispatcher0.waitForParallelFinish();
        dispatcher1.waitForParallelFinish();
    }
}
