#include "../../../utils/timer_delegate/timer_delegate.h"

#include <gtest/gtest.h>

#include <future>

#include "../../../utils/timer_delegate/itimer_strategy.h"

TEST(TimerDelegateTest, NullptrTest)
{
    TimerDelegate td{};
    EXPECT_THROW(td.start(nullptr), std::invalid_argument);
}

TEST(TimerDelegateTest, DoubleStart)
{
    class FirstStrategy final : public ITimerStrategy
    {
       public:
        int nextTimeout() override { return 150; }

        void onTimeout() override {}
    };

    TimerDelegate td{};

    td.start(std::make_unique<FirstStrategy>());
    EXPECT_THROW(td.start(std::make_unique<FirstStrategy>()), std::runtime_error);
}

TEST(TimerDelegateTest, CheckTimeout)
{
    struct Promise
    {
        std::promise<void> p;
        std::atomic<bool> called{false};

        void set()
        {
            bool expected = false;
            if (called.compare_exchange_strong(expected, true))
            {
                try
                {
                    p.set_value();
                }
                catch (...)
                {
                }
            }
        }

        std::future<void> get_future() { return p.get_future(); }
    } p{};

    struct FirstStrategy final : public ITimerStrategy
    {
        FirstStrategy(Promise& p) : p(p) {}
        int nextTimeout() override { return 150; }
        void onTimeout() override { p.set(); }

        Promise& p;
    };

    auto fut = p.get_future();

    TimerDelegate td;
    td.start(std::make_unique<FirstStrategy>(p));
    ASSERT_EQ(std::future_status::ready, fut.wait_for(std::chrono::milliseconds(1000)));
    td.stop();
}

TEST(TimerDelegateTest, CheckReset)
{
    struct StrategyWithCounter final : public ITimerStrategy
    {
        int nextTimeout() override { return 100; }
        void onTimeout() override { ++count; }
        std::atomic<int> count{0};
    };

    TimerDelegate td;
    auto strategy = new StrategyWithCounter();
    td.start(std::unique_ptr<ITimerStrategy>(strategy));

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    td.reset();

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_EQ(0, strategy->count.load());

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(1, strategy->count.load());

    td.stop();
}
