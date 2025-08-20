#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <queue>
#include <atomic>

class ThreadPool final{
public:
    explicit ThreadPool(size_t amount);
    ~ThreadPool();

    void add_task(std::function<void()> task);
    void wait();

private:
    size_t amount_;
    std::queue<std::function<void()>> tasks_;
    std::vector<std::thread> workers_;
    std::condition_variable cv_;
    std::mutex mtx_;
    std::atomic<bool> stop_ = false;
};
