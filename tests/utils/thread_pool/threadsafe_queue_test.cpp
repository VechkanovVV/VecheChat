#include "threadsafe_queue.h"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>

TEST(ThreadsafeQueueTest, FifoOrderPushPop)
{
    utils::ThreadsafeQueue<int> tsq;
    tsq.push(1);
    tsq.push(2);

    EXPECT_EQ(*tsq.wait_and_pop(), 1);
    EXPECT_EQ(*tsq.wait_and_pop(), 2);

    tsq.push(3);
    tsq.push(4);
    int val1{-1}, val2{-1};
    tsq.wait_and_pop(val1);
    tsq.wait_and_pop(val2);

    EXPECT_EQ(val1, 3);
    EXPECT_EQ(val2, 4);
}

TEST(ThreadsafeQueueTest, PushAfterStopIgnored)
{
    utils::ThreadsafeQueue<int> tsq;

    bool v1 = tsq.push(1);
    EXPECT_TRUE(v1);

    tsq.stop();

    bool v2 = tsq.push(2);
    EXPECT_FALSE(v2);
}

TEST(ThreadsafeQueueTest, WaitAndPopWakesOnPush)
{
    utils::ThreadsafeQueue<int> tsq;
    std::atomic<bool> value_received{false};
    std::atomic<bool> thread_started{false};
    std::thread t(
        [&]
        {
            thread_started.store(true);
            int value;
            value_received.store(tsq.wait_and_pop(value));
        });

    while (!thread_started.load()) std::this_thread::yield();

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    EXPECT_FALSE(value_received.load());

    tsq.push(1);
    tsq.stop();
    t.join();

    EXPECT_TRUE(value_received.load());
}
