#include "timer_delegate.h"

#include <chrono>
#include <stdexcept>

utils::TimerDelegate::~TimerDelegate()
{
    stop();
}

void utils::TimerDelegate::start(std::unique_ptr<ITimerStrategy> strategy)
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
        current_strategy_ = std::shared_ptr<ITimerStrategy>(std::move(strategy));
        strategy_changed_.store(true);
        reset_flag_.store(false);
        stop_flag_.store(false);
    }

    worker_ = std::thread(&TimerDelegate::runLoop, this);
}

void utils::TimerDelegate::stop()
{
    {
        std::lock_guard lk(mutex_);
        if (stop_flag_.load()) return;
        stop_flag_.store(true);
    }

    cv_.notify_one();

    if (worker_.joinable() && worker_.get_id() != std::this_thread::get_id())
    {
        worker_.join();
    }
}

void utils::TimerDelegate::reset()
{
    {
        std::lock_guard lk(mutex_);
        reset_flag_.store(true);
    }

    cv_.notify_one();
}

void utils::TimerDelegate::changeStrategy(std::unique_ptr<ITimerStrategy> strategy)
{
    {
        std::lock_guard lk(mutex_);

        if (!strategy)
        {
            throw std::invalid_argument("Strategy must not be nullptr!");
        }
        current_strategy_ = std::shared_ptr<ITimerStrategy>(std::move(strategy));
        strategy_changed_.store(true);
    }

    cv_.notify_one();
}

void utils::TimerDelegate::runLoop()
{
    while (true)
    {
        {
            std::unique_lock lk(mutex_);
            if (stop_flag_.load()) break;

            cv_.wait(lk,
                     [&] {
                         return stop_flag_.load() || current_strategy_ != nullptr || reset_flag_.load() ||
                                strategy_changed_.load();
                     });

            if (stop_flag_.load()) break;

            std::shared_ptr<ITimerStrategy> strategy_to_use = current_strategy_;

            if (!strategy_to_use)
            {
                continue;
            }
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
                    // Exception in onTimeout() is intentionally ignored to prevent timer thread from terminating.
                }
            }
        }
    }
}