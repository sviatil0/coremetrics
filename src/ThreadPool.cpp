#include "ThreadPool.hpp"
#include <thread>

ThreadPool::ThreadPool() : stop(false)
{
    size_t n = std::thread::hardware_concurrency();
    if (n == 0)
    {
        n = 2;
    }
    workers.reserve(n);
    for (size_t i = 0; i < n; ++i)
    {
        workers.emplace_back([this]()
        {
            while (true)
            {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(queueMutex);
                    cv.wait(lock, [this]()
                    {
                        return stop || !tasks.empty();
                    });

                    if (stop && tasks.empty())
                    {
                        return;
                    }
                    
                    task = std::move(tasks.front());
                    tasks.pop();
                }
                task();
            }
        });
    }
}

ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        stop = true;
    }
    cv.notify_all();
    for (std::thread& worker : workers)
    {
        worker.join();
    }
}

ThreadPool& ThreadPool::getInstance()
{
    static ThreadPool instance;
    return instance;
}

size_t ThreadPool::threadCount() const
{
    return workers.size();
}
