#pragma once

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <type_traits>
#include <utility>

namespace utils
{
// ThreadsafeQueue<T>
// A thread-safe FIFO queue.
// - push(U&&): inserts an element; if the queue has been stopped (stop() called), push is ignored.
// - stop(): marks the queue as closed and wakes all waiters; no new elements are accepted after stop().
//   Elements that were already enqueued before stop() can still be popped.
// - wait_and_pop / try_pop: retrieve elements; if the queue is closed and empty, wait_and_pop returns false / nullptr.
// Contract: before destroying the queue the user must either call stop() or ensure no threads are blocked in
// wait_and_pop.
template <typename T>
class ThreadsafeQueue final
{
   private:
    mutable std::mutex mtx_;
    std::queue<T> data_queue_;
    std::condition_variable data_convar_;
    bool stopped_ = false;

   public:
    ThreadsafeQueue() = default;
    ~ThreadsafeQueue() = default;
    ThreadsafeQueue(const ThreadsafeQueue&) = delete;
    ThreadsafeQueue& operator=(const ThreadsafeQueue&) = delete;

    template <typename U, std::enable_if_t<std::is_constructible_v<T, U&&>, int> = 0>
    bool push(U&& new_value)
    {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            if (stopped_)
            {
                return false;
            }
            data_queue_.push(std::forward<U>(new_value));
        }
        data_convar_.notify_one();
        return true;
    }

    void stop()
    {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            stopped_ = true;
        }
        data_convar_.notify_all();
    }

    bool wait_and_pop(T& value)
    {
        std::unique_lock<std::mutex> lock(mtx_);
        data_convar_.wait(lock, [&] { return stopped_ || !data_queue_.empty(); });
        if (stopped_ && data_queue_.empty())
        {
            return false;
        }
        value = std::move(data_queue_.front());
        data_queue_.pop();
        return true;
    }

    std::shared_ptr<T> wait_and_pop()
    {
        std::unique_lock<std::mutex> lock(mtx_);
        data_convar_.wait(lock, [&] { return stopped_ || !data_queue_.empty(); });
        if (stopped_ && data_queue_.empty())
        {
            return nullptr;
        }
        auto res = std::make_shared<T>(std::move(data_queue_.front()));
        data_queue_.pop();
        return res;
    }

    bool try_pop(T& value)
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (data_queue_.empty())
        {
            return false;
        }

        value = std::move(data_queue_.front());
        data_queue_.pop();
        return true;
    }

    std::shared_ptr<T> try_pop()
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (data_queue_.empty())
        {
            return nullptr;
        }

        auto res = std::make_shared<T>(std::move(data_queue_.front()));
        data_queue_.pop();
        return res;
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return data_queue_.empty();
    }
};
};  // namespace utils
