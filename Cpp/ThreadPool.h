#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>
#include <iostream>
#include <errno.h>
#include <strings.h>
#include <string.h>

using namespace std;
const int TP_AUTO = 0;

class ThreadPool {
public:
    ThreadPool(int threads = TP_AUTO, int priority_orNegativeToBackgorundPool_orZeroToDefault = TP_AUTO, string name_max_15chars = "");
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>;
    // the task queue
    ~ThreadPool();
    int getTaskCount();


    string tag;
private:
    string threadsNames;
    int tasksCounter = 0;
    int threadCount = 0;
    int maxThreads = 0;
    int poolPriority = 0;
    // need to keep track of threads so we can join them
    std::vector<std::thread > workers;

    std::vector<string>threadNames;


    std::queue< std::function<void()> > tasks;
    // synchronization
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
    void NewThread();
};

// add new work item to the pool
template<class F, class... Args> auto ThreadPool::enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>
{
    using return_type = typename std::result_of<F(Args...)>::type;

    auto task = std::make_shared< std::packaged_task<return_type()> >(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(queue_mutex);

        // don't allow enqueueing after stopping the pool
        if(stop)
            throw std::runtime_error("enqueue on stopped ThreadPool");

        tasks.emplace([task](){ (*task)(); });
    }
    condition.notify_one();

    this->tasksCounter++;

    if ((this->tasksCounter > this->threadCount && this->maxThreads == TP_AUTO) || this->threadCount == 0)
    {
        this->NewThread();
    }
    return res;
}

#endif
