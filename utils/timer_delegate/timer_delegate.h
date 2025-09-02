#pragma once
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

#include "itimer_strategy.h"

namespace utils
{

/// TimerDelegate is responsible for running a timer loop in a separate thread.
/// It manages a timer strategy (ITimerStrategy) that defines when and what
/// action to execute. The delegate supports starting, stopping, resetting,
/// and switching strategies in a thread-safe manner.
class TimerDelegate final
{
   public:
    TimerDelegate() = default;
    ~TimerDelegate();

    TimerDelegate(const TimerDelegate&) = delete;
    TimerDelegate& operator=(const TimerDelegate&) = delete;

    TimerDelegate(TimerDelegate&&) = delete;
    TimerDelegate& operator=(TimerDelegate&&) = delete;

    void start(std::unique_ptr<utils::ITimerStrategy> strategy);
    void stop();
    void reset();
    void changeStrategy(std::unique_ptr<utils::ITimerStrategy> strategy);

   private:
    void runLoop();

    std::mutex mutex_;
    std::condition_variable cv_;
    std::thread worker_;
    std::shared_ptr<utils::ITimerStrategy> current_strategy_;

    std::atomic<bool> stop_flag_{false};
    std::atomic<bool> reset_flag_{false};
    std::atomic<bool> strategy_changed_{false};
};
};  // namespace utils