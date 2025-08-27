#include "TimerDelegate.h"

#include <chrono>
#include <stdexcept>

TimerDelegate::~TimerDelegate()
{
    stop();
}

void TimerDelegate::start(std::unique_ptr<ITimerStrategy> strategy)
{
    if (!strategy)
    {
        throw std::invalid_argument("Strategy must not be nullptr!");
    }

    if (worker_.joinable())
    {
        throw std::runtime_error("Timer has been already started!");
    }

    {
        std::lock_guard lk(mutex_);
        current_strategy_ = std::move(strategy);
        strategy_changed_.store(true);
        reset_flag_.store(false);
        stop_flag_.store(false);
    }

    worker_ = std::thread(&TimerDelegate::runLoop, this);
    cv_.notify_one();
}

void TimerDelegate::stop()
{
    {
        std::lock_guard lk(mutex_);
        stop_flag_.store(true);
    }

    cv_.notify_one();

    if (worker_.joinable())
    {
        worker_.join();
    }
}

void TimerDelegate::reset()
{
    {
        std::lock_guard lk(mutex_);
        reset_flag_.store(true);
    }

    cv_.notify_one();
}

void TimerDelegate::changeStrategy(std::unique_ptr<ITimerStrategy> strategy)
{
    {
        std::lock_guard lk(mutex_);
        current_strategy_ = std::move(strategy);
        strategy_changed_.store(true);
    }

    cv_.notify_one();
}

void TimerDelegate::runLoop()
{
    while (true)
    {
        {
            std::unique_lock lk(mutex_);
            if (stop_flag_.load()) break;

            cv_.wait(lk, [&] { return stop_flag_.load() || current_strategy_ != nullptr; });

            if (stop_flag_.load()) break;

            ITimerStrategy* strategy_to_use = current_strategy_.get();

            if (strategy_changed_.load() || reset_flag_.load())
            {
                strategy_changed_.store(false);
                reset_flag_.store(false);
                continue;
            }

            const int timeout = strategy_to_use->nextTimeout();
            if (timeout < 0)
            {
                cv_.wait(lk, [&] { return stop_flag_.load() || strategy_changed_.load() || reset_flag_.load(); });
                continue;
            }

            const bool expired =
                !cv_.wait_for(lk, std::chrono::milliseconds(timeout),
                              [&] { return stop_flag_.load() || reset_flag_.load() || strategy_changed_.load(); });

            if (stop_flag_.load()) break;
            if (strategy_changed_.load() || reset_flag_.load()) continue;

            if (expired)
            {
                lk.unlock();
                try
                {
                    strategy_to_use->onTimeout();
                }
                catch (...)
                {
                }
            }
        }
    }
}