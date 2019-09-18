#include "ThreadPool.h"

// the constructor just launches some amount of workers
ThreadPool::ThreadPool(size_t threads, int priority_orNegativeToBackgorundPool_orZeroToDefault, string name_max_15chars):stop(false)
{
    for(size_t i = 0;i<threads;++i)
    {
        //workers.emplace_back(
        std::thread tmpThread(
            [&]()
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

        //workers.push_back(tmpThread);
        auto tid = tmpThread.native_handle();

        if (name_max_15chars != "")
        {
            string tmpName = name_max_15chars + std::to_string(i);

            if (tmpName.size() > 15)
                tmpName = tmpName.substr(0, 15);



            pthread_setname_np(tmpThread.native_handle(), tmpName.c_str());
        }

        if (priority_orNegativeToBackgorundPool_orZeroToDefault != 0)
        {
            //https://access.redhat.com/documentation/en-US/Red_Hat_Enterprise_MRG/1.3/html/Realtime_Reference_Guide/chap-Realtime_Reference_Guide-Priorities_and_policies.html/
            int policy;
            sched_param sch;
            pthread_getschedparam(tid, &policy, &sch);
            sch.sched_priority = priority_orNegativeToBackgorundPool_orZeroToDefault;


            if (priority_orNegativeToBackgorundPool_orZeroToDefault > 0)
            {
                if (pthread_setschedparam(tid, SCHED_FIFO, &sch)) {
                    std::cout << "Failed to setschedparam: " << strerror(errno) << '\n';
                }
            }
            else if (priority_orNegativeToBackgorundPool_orZeroToDefault < 0)
            {
                cout << "Extreme low priority pool created." << endl;
                if (pthread_setschedparam(tid, SCHED_IDLE, &sch)) {
                    std::cout << "Failed to setschedparam: " << strerror(errno) << '\n';
                }
            }
        }

        //workers.back().detach();
        tmpThread.detach();


    }
}



// the destructor joins all threads
ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    for(std::thread &worker: workers)
        worker.join();
}

int ThreadPool::getTaskCount()
{
    return this->tasksCounter;
}
