#include "thread_pool.h"
#include <stdexcept>

ThreadPool::ThreadPool(size_t amount) : amount_(amount) {
    for (size_t i = 0; i < amount_; ++i) {
        workers_.emplace_back([this]{
            while(!stop_.load()) {
                std::function<void()> task;
                {
                    std::unique_lock lock(mtx_);
                    cv_.wait(lock, [this]{
                        return !tasks_.empty() || stop_.load();
                    });

                    if (stop_ && tasks_.empty()) {
                        return;
                    }

                    task = std::move(tasks_.front());
                    tasks_.pop();
                }
                task();
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::lock_guard lock(mtx_);
        stop_.store(true);
        cv_.notify_all();
    }

    wait();
}

void ThreadPool::wait() {
    for (auto& w : workers_) {
        if (w.joinable()) {
            w.join();
        }
    }
}

void ThreadPool::add_task(std::function<void()> task) {
    {
        std::lock_guard lock(mtx_);

        if (stop_.load()) {
            throw std::runtime_error("ThreadPool has been stopped!");
        }
        tasks_.emplace(std::move(task));
    }
    cv_.notify_one();
}