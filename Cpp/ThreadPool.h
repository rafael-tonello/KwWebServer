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

class ThreadPool {
public:
    ThreadPool(size_t threads, int priority_orNegativeToBackgorundPool_orZeroToDefault);
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>;
    // the task queue
    ~ThreadPool();

    int getTaskCount();
    int tasksCounter = 0;
    string tag;
private:
    // need to keep track of threads so we can join them
    std::vector< std::thread > workers;


    std::queue< std::function<void()> > tasks;
    // synchronization
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};

// the constructor just launches some amount of workers
inline ThreadPool::ThreadPool(size_t threads, int priority_orNegativeToBackgorundPool_orZeroToDefault):stop(false)
{
    for(size_t i = 0;i<threads;++i)
    {
        workers.emplace_back(
            [this]
            {
                for(;;)
                {
                    std::function<void()> task;

                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock, [this]{ return this->stop || !this->tasks.empty(); });
                        if(this->stop && this->tasks.empty())
                            return;

                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }

                    task();
                    this->tasksCounter--;
                }
            }
        );

        if (priority_orNegativeToBackgorundPool_orZeroToDefault != 0)
        {
            int policy;
            sched_param sch;
            pthread_getschedparam(workers.back().native_handle(), &policy, &sch);
            sch.sched_priority = priority_orNegativeToBackgorundPool_orZeroToDefault;

            if (priority_orNegativeToBackgorundPool_orZeroToDefault > 0)
            {
                if (pthread_setschedparam(workers.back().native_handle(), SCHED_FIFO, &sch)) {
                    std::cout << "Failed to setschedparam: " << strerror(errno) << '\n';
                }
            }
            else
            {
                cout << "Extreme low priority pool created." << endl;
                if (pthread_setschedparam(workers.back().native_handle(), SCHED_IDLE, &sch)) {
                    std::cout << "Failed to setschedparam: " << strerror(errno) << '\n';
                }
            }
        }
    }
}

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
    return res;
}

// the destructor joins all threads
inline ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    for(std::thread &worker: workers)
        worker.join();
}

inline int ThreadPool::getTaskCount()
{
    return this->tasksCounter;
}
#endif
