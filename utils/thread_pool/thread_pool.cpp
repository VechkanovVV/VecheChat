#include "thread_pool.h"

utils::ThreadPool::ThreadPool(size_t thread_count) : thread_count_(thread_count)
{
    for (size_t i = 0; i < thread_count_; ++i)
    {
        workers_.emplace_back(
            [this]
            {
                while (true)
                {
                    std::function<void()> task;
                    auto popped = tasks_.wait_and_pop(task);
                    if (popped)
                        task();
                    else
                    {
                        break;
                    }
                }
            });
    }
}

utils::ThreadPool::~ThreadPool()
{
    stop_and_wait();
}

void utils::ThreadPool::stop_and_wait()
{
    tasks_.stop();

    for (auto& w : workers_)
    {
        if (w.joinable())
        {
            w.join();
        }
    }
}

void utils::ThreadPool::add_task(std::function<void()> task)
{
    tasks_.push(task);
}