#pragma once

/// Timer "Strategy" interface: defines after how many milliseconds to trigger
/// and what action to perform on trigger.

namespace utils
{
class ITimerStrategy
{
   public:
    virtual ~ITimerStrategy() = default;
    virtual int nextTimeout() = 0;
    virtual void onTimeout() = 0;
};
};  // namespace utils