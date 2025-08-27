#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

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
    std::queue<std::function<void()>> tasks_;
    std::vector<std::thread> workers_;
    std::condition_variable cv_;
    std::mutex mtx_;
    std::atomic<bool> stop_ = false;
};
