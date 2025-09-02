#include "../../../utils/thread_pool/thread_pool.h"

#include <gtest/gtest.h>

#include <atomic>
#include <future>
#include <iostream>
#include <thread>

TEST(ThreadPoolTest, AllTasksAreExecuted)
{
    auto n = std::thread::hardware_concurrency();
    utils::ThreadPool tp{n};
    std::atomic<int> counter{0};
    for (size_t i = 0; i < 100; ++i)
    {
        tp.add_task([&counter] { counter.fetch_add(1); });
    }

    tp.stop_and_wait();

    EXPECT_EQ(counter, 100);
}

TEST(ThreadPoolTest, GetReturnValue)
{
    auto n = std::thread::hardware_concurrency();
    utils::ThreadPool tp{n};

    auto f1 = tp.add_task(
        []() -> int
        {
            std::this_thread::sleep_for(std::chrono::microseconds(20));
            return 0;
        });

    auto f2 = tp.add_task(
        []() -> std::string
        {
            std::this_thread::sleep_for(std::chrono::microseconds(20));
            return "test";
        });
    EXPECT_EQ(f1.get(), 0);
    EXPECT_EQ(f2.get(), "test");
}