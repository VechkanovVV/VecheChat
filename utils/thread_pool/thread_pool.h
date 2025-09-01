#pragma once

#include <functional>
#include <thread>
#include <vector>

#include "threadsafe_queue.h"

namespace utils
{
/// ThreadPool — a small, thread-safe thread pool for executing tasks concurrently.
/// Constructor creates `thread_count` worker threads. Use `add_task()` to schedule
/// tasks (std::function<void()>) and `wait()` to block until all scheduled work is done.
class ThreadPool final
{
   public:
    explicit ThreadPool(size_t thread_count);
    ~ThreadPool();

    void add_task(std::function<void()> task);
    void stop_and_wait();

   private:
    size_t thread_count_;
    ThreadsafeQueue<std::function<void()>> tasks_;
    std::vector<std::thread> workers_;
};
};  // namespace utils
