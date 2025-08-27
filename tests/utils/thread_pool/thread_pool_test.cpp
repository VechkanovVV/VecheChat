#include "../../../utils/thread_pool/thread_pool.h"

#include <gtest/gtest.h>

#include <atomic>
#include <iostream>
#include <thread>

TEST(ThreadPoolTest, test)
{
    auto n = std::thread::hardware_concurrency();
    ThreadPool tp{n};
    std::atomic<int> counter{0};
    for (size_t i = 0; i < 100; ++i)
    {
        tp.add_task([&counter] { counter.store(counter.load() + 1); });
    }

    tp.stop_and_wait();

    EXPECT_EQ(counter, 100);
}