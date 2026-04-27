#ifndef __THREADPOOL_HPP__
#define __THREADPOOL_HPP__

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <stdexcept>

class ThreadPool
{
public:
    ~ThreadPool();
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    static ThreadPool& getInstance();
    size_t threadCount() const;

    template<typename F>
    std::future<void> submit(F&& f)
    {
        std::shared_ptr<std::packaged_task<void()>> task = std::make_shared<std::packaged_task<void()>>(std::forward<F>(f));
        std::future<void> result = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            
            if (stop)
            {
                throw std::runtime_error("ThreadPool has been stopped");
            }
            
            tasks.emplace([task]()
            {
                (*task)();
            });
        }
        
        cv.notify_one();
        return result;
    }

private:
    ThreadPool();

    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queueMutex;
    std::condition_variable cv;
    
    bool stop;
};

#endif
