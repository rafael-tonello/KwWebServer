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
    ThreadPool(int threads = TP_AUTO, int priority_orNegativeToBackgorundPool_orZeroToDefault = TP_AUTO, string name_max_15chars = "", bool forceThreadCreationAtStartup = false);
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>;
    // the task queue
    ~ThreadPool();
    int getTaskCount();
    int getTotalDoneTasks();

    std::mutex newThreadMutex;


    string tag;
private:
    string threadsNames;
    unsigned int doneTasks = 0;
    int tasksCounter = 0;
    int threadCount = 0;
    int maxThreads = 0;
    int poolPriority = 0;
    //this variable is used to determine if new trheads mus be created...
    //If buffer conains 100 tasks, and only 9 threads is running, a new must be created
    //this variable only works if the maxThread is 0 (automatic number of threads)
    int bufferSizeBeforeNewThreads = 10;
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



    if
    (
        (
            this->threadCount == 0
        ) ||
        (
            this->tasksCounter > this->bufferSizeBeforeNewThreads &&
            (
                this->threadCount < this->maxThreads ||
                this->maxThreads == TP_AUTO
            )
        )
    )
    {
        this->NewThread();
    }
    return res;
}

#endif
