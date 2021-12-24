#include "ThreadPool.h"

// the constructor just launches some amount of workers
ThreadPool::ThreadPool(
    int threads,
    int niceValue,
    string name_max_15chars,
    bool forceThreadCreationAtStartup
):stop(false)
{
    /*if (priority_orNegativeToBackgorundPool_orZeroToDefault < 0)
        this->schedul_policy = SCHED_IDLE;
    if (priority_orNegativeToBackgorundPool_orZeroToDefault > 0)
        this->schedul_policy = SCHED_FIFO;*/

    if (niceValue < -20)
    {
        this->schedul_policy = SCHED_IDLE;
        niceValue = 0;
    }
    else{
        this->schedul_policy = SCHED_OTHER;
    }

    this->threadsNames = name_max_15chars;
    this->maxThreads = threads;
    this->poolPriority = niceValue;

    if(forceThreadCreationAtStartup)
    {
        for(int i = 0;i<threads;++i)
        {
            this->NewThread();
        }
    }
}

ThreadPool::ThreadPool(
    bool forceThreadCreationAtStartup,
    int threads,
    int priorityOrNiceValue,
    int scheduling_policy,
    string name_max_15chars
):stop(false)
{
    this->schedul_policy = scheduling_policy;

    this->threadsNames = name_max_15chars;
    this->maxThreads = threads;
    this->poolPriority = priorityOrNiceValue;

    if(forceThreadCreationAtStartup)
    {
        for(int i = 0;i<threads;++i)
        {
            this->NewThread();
        }
    }
}

void ThreadPool::NewThread()
{
    this->newThreadMutex.lock();
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

                    currentBusyThreads++;
                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                }

                task();
                currentBusyThreads--;
                this->doneTasks++;
                this->tasksCounter--;
            }
        }

    );

    //workers.push_back(tmpThread);
    auto tid = tmpThread.native_handle();

    if (this->threadsNames!= "")
    {
        string tmpName = this->threadsNames + std::to_string(this->threadCount);

        if (tmpName.size() > 15)
            tmpName = tmpName.substr(0, 15);

        pthread_setname_np(tmpThread.native_handle(), tmpName.c_str());

    }

    if (this->poolPriority != 0)
    {
        //https://access.redhat.com/documentation/en-US/Red_Hat_Enterprise_MRG/1.3/html/Realtime_Reference_Guide/chap-Realtime_Reference_Guide-Priorities_and_policies.html/
        int policy;
        sched_param sch;
        pthread_getschedparam(tid, &policy, &sch);
        sch.sched_priority = this->poolPriority;

        if (this->schedul_policy != SCHED_OTHER)
        {
            if (pthread_setschedparam(tid, this->schedul_policy, &sch)) {
                std::cout << "Failed to setschedparam: " << strerror(errno) << '\n';
            }
        }
        else{
            nice(this->poolPriority);
        }

        if (this->schedul_policy == SCHED_IDLE)
            cout << "Extreme low priority pool created." << endl;
    }


    //workers.back().detach();
    tmpThread.detach();
    //workers.push_back(tmpThread);
    //cout << "Create the thread " <<  this->threadsNames << this->threadCount << endl;

    this->threadCount++;

    this->newThreadMutex.unlock();

}

int ThreadPool::getTotalDoneTasks()
{
    return this->doneTasks;
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
    //return this->tasksCounter;
    return this->tasks.size();
}
