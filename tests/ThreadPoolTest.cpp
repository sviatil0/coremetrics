#include <iostream>
#include <atomic>
#include <vector>
#include <future>
#include "ThreadPoolTest.hpp"
#include "ThreadPool.hpp"

void testThreadPoolSingleton()
{
    std::cout << "ThreadPool : getInstance() returns same instance: ";
    ThreadPool &first = ThreadPool::getInstance();
    ThreadPool &second = ThreadPool::getInstance();
    bool passed = (&first == &second);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testThreadPoolThreadCount()
{
    std::cout << "ThreadPool : threadCount() is greater than zero: ";
    ThreadPool &pool = ThreadPool::getInstance();
    bool passed = (pool.threadCount() > 0);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testThreadPoolSubmitExecutes()
{
    std::cout << "ThreadPool : submitted task executes: ";
    ThreadPool &pool = ThreadPool::getInstance();
    std::atomic<int> flag(0);
    std::future<void> f = pool.submit([&flag]()
    {
        flag.store(1);
    });
    f.get();
    bool passed = (flag.load() == 1);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testThreadPoolMultipleTasks()
{
    std::cout << "ThreadPool : multiple tasks all complete: ";
    ThreadPool &pool = ThreadPool::getInstance();
    std::atomic<int> counter(0);
    std::vector<std::future<void>> futures;
    for (int i = 0; i < 10; ++i)
    {
        futures.push_back(pool.submit([&counter]()
        {
            counter.fetch_add(1);
        }));
    }
    for (std::future<void>& f : futures)
    {
        f.get();
    }
    bool passed = (counter.load() == 10);
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void testThreadPoolFutureValid()
{
    std::cout << "ThreadPool : submit() returns a valid future: ";
    ThreadPool &pool = ThreadPool::getInstance();
    std::future<void> f = pool.submit([]() {});
    bool passed = f.valid();
    f.get();
    std::cout << (passed ? "PASS" : "FAIL") << '\n';
}

void threadPoolTestSuite()
{
    std::cout << "=============================================" << '\n';
    std::cout << "  ThreadPool : Unit Tests" << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << '\n';

    testThreadPoolSingleton();
    testThreadPoolThreadCount();
    testThreadPoolSubmitExecutes();
    testThreadPoolMultipleTasks();
    testThreadPoolFutureValid();

    std::cout << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << "  ThreadPool tests completed!" << '\n';
    std::cout << "=============================================" << '\n';
}
