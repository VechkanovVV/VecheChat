#include "thread_pool.h"

#include <stdexcept>

ThreadPool::ThreadPool(size_t thread_count) : thread_count_(thread_count)
{
    for (size_t i = 0; i < thread_count_; ++i)
    {
        workers_.emplace_back(
            [this]
            {
                while (true)
                {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(mtx_);
                        cv_.wait(lock, [this] { return !tasks_.empty() || stop_.load(); });

                        if (stop_.load() && tasks_.empty())
                        {
                            return;
                        }

                        if (!tasks_.empty())
                        {
                            task = std::move(tasks_.front());
                            tasks_.pop();
                        }
                        else
                        {
                            continue;
                        }
                    }
                    task();
                }
            });
    }
}

ThreadPool::~ThreadPool()
{
    {
        std::lock_guard lock(mtx_);
        stop_.store(true, std::memory_order_release);
        cv_.notify_all();
    }

    stop_and_wait();
}

void ThreadPool::stop_and_wait()
{
    stop_.store(true, std::memory_order_acquire);
    cv_.notify_all();

    for (auto& w : workers_)
    {
        if (w.joinable())
        {
            w.join();
        }
    }
}

void ThreadPool::add_task(std::function<void()> task)
{
    {
        std::lock_guard lock(mtx_);

        if (stop_.load(std::memory_order_acquire))
        {
            throw std::runtime_error("ThreadPool has been stopped!");
        }
        tasks_.emplace(std::move(task));
    }
    cv_.notify_one();
}