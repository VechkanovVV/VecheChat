#pragma once

#include <functional>
#include <future>
#include <thread>
#include <vector>

#include "threadsafe_queue.h"

namespace utils
{
/// ThreadPool — a thread-safe pool executing tasks concurrently.
/// `add_task()` schedules a task and returns a `std::future` for its result.
/// `stop_and_wait()` waits for all tasks to finish.

class ThreadPool final
{
   public:
    explicit ThreadPool(size_t thread_count);
    ~ThreadPool();

    template <typename F, typename... Args>
    std::future<std::invoke_result_t<F, Args...>> add_task(F&& f, Args&&... args)
    {
        using R = std::invoke_result_t<F, Args...>;
        auto task = std::make_shared<std::packaged_task<R()>>(
            [func = std::forward<F>(f), tup = std::make_tuple(std::forward<Args>(args)...)]() mutable
            { return std::apply(func, std::move(tup)); });

        auto res = task->get_future();
        tasks_.push([task] { (*task)(); });
        return res;
    }

    void stop_and_wait();

   private:
    size_t thread_count_;
    ThreadsafeQueue<std::function<void()>> tasks_;
    std::vector<std::thread> workers_;
};
};  // namespace utils
