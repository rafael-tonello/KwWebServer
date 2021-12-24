#ifndef __JsPromise_H_
#include "JsPromise.h"
#endif

void Promise::debug(string msg)
{

//    cout << "[debug, "+this->promiseName+"] " << msg << endl << flush;
}

ThreadPool *Promise::tasker = NULL;
bool Promise::initDone = false;

mutex Promise::__garbageLocker;
vector<Promise*> Promise::__garbage;

void Promise::initLib(ThreadPool *nTasker)
{
    if (!Promise::initDone)
    {
        Promise::initDone = true;
        
        if (nTasker != NULL)
            tasker = nTasker;
        else
            tasker = new ThreadPool();
        
        std::thread th([](){
            while (true){
                usleep(2000000);
                cout << "Garbage cicle " << endl << flush;
                Promise::__garbageLocker.lock();
                for (int c = 0; c < Promise::__garbage.size(); c++)
                {
                    Promise* t = Promise::__garbage[c];
                    delete t;
                    
                    //run this thread in very slow speed;
                    //usleep(1000);
                }
                Promise::__garbage.clear();
                Promise::__garbageLocker.unlock();
            }
        });
        th.detach();
    }
}


Promise::Promise(function<void()> mainFunction, string debugName)
{
    if (tasker == NULL)
    {
        Promise::initLib();
    }
    
    this->promiseName = debugName;
    this->then(mainFunction);
}
        
Promise* Promise::then(function<void()> callback)
{
    if (this->stopped)
    {
        cout << "Try running 'then' in stopped Promise" << endl << flush;
        return this;
    }

    debug("entered in the then function");
    this->thenLocker.lock();
    this->thens.push_back(callback);
    this->thenLocker.unlock();
    debug("running run function");
    this->run();
    debug("Ok, exiting from then function");
    return this;
}


void Promise::run()
{
    if (this->stopped)
    {
        cout << "Try running 'run' in stopped Promise" << endl << flush;
        return;
    }
    
    debug("enqueuing a new task");
    tasker->enqueue([this](){
        debug("entered in the run");
        function<void()> nextThen = NULL;
        bool reRun = false;
        
        debug("run prepared");
        if (runIsRunning)
            return;
            
        runIsRunning = true;    
        
        //get next then
        debug("Will lock the semaphore");
        this->thenLocker.lock();
        debug("semaphore locked");
        if (this->thens.size() > 0)
        {
            nextThen = this->thens[0];
            this->thens.erase(this->thens.begin());
        }
        this->thenLocker.unlock();
        debug("semaphore unlocked");
        if (nextThen != NULL)
        {
            debug("will run then");
            nextThen();
            debug("then runned");
        }
        
        runIsRunning =false;
        
        //checks if there is more thens to run
        this->thenLocker.lock();
        if (this->thens.size() > 0)
        {
            reRun = true;
        }
        this->thenLocker.unlock();
        
        if (reRun)
            this->run();
    });
    
    debug("task enqueued");
}


void Promise::finalize()
{
    if (this->stopped)
    {
        cout << "Try running 'finalize' in stopped Promise" << endl << flush;
        return;
    }

    this->stopped = true;
    Promise::__garbageLocker.lock();
    Promise::__garbage.push_back(this);
    Promise::__garbageLocker.unlock();
}



#include <string>
int main1();
int main2();

/*int main()
{
    return main1();
}*/

int main1(){



    cout << "First main example " << endl << flush;

    for (int count = 0; count < 1000; count++)
    {
        cout << "==================" << count << "===============" << endl << flush;


        int dones = 0;
        auto a = new Promise([](){
            cout << "First text" << endl << flush;
            usleep(500000);
        }, "promise 1");

        
        a->then([](){
            cout << "Second text" << endl << flush;
            usleep(1000000);
        });

        a->then([](){
            cout << "Third text" << endl << flush;
        });


      /*  auto b = new Promise([](){
            cout << "c1" << endl << flush;
        }, "promise 2");
        
        b->then([](){
            cout << "c2 " << endl << flush;
        })->then([](){

            cout << "c3 " << endl << flush;
        })->then([](){

            cout << "c4 " << endl << flush;
        })->then([&dones](){
            cout << "c5 " << endl << flush;
            dones++;
        })->finalize([&dones]{
        });
*/


        a->then([a, &dones](){
            cout << "Forth text         " << endl << flush;
            dones++;

            a->finalize();
        });


        while (dones < 1)
        {
            usleep(50000);
        }

    }

    cout << "Program end " << endl << flush;
}

int main2(){
    cout << "Secong main example " << endl <<flush;
    ThreadPool *th = new ThreadPool();
    
    cout << "First main example " << endl << flush;

    for (int count = 0; count < 100; count++)
    {
        cout << "==================" << count << "===============" << endl << flush;


        int dones = 0;
        th->enqueue([th, &dones](){
            cout << "First text" << endl << flush;
            usleep(500000);
            th->enqueue([th, &dones](){
                cout << "Second text" << endl << flush;
                usleep(1000000);
                
                th->enqueue([th, &dones](){
                    cout << "Third text" << endl << flush;
                    
                    th->enqueue([&dones](){
                        cout << "Forth text " << endl << flush;
                        
                        dones++;
                    });
                    
                    
                });
            });

        });

        th->enqueue([th, &dones](){
            cout << "c1" << endl << flush;
        
            th->enqueue([th, &dones](){
                cout << "c2 " << endl << flush;
                
                th->enqueue([th, &dones](){
                    cout << "c3 " << endl << flush;
                    
                    th->enqueue([th, &dones](){
                        cout << "c4 " << endl << flush;
                        th->enqueue([th, &dones](){
                            cout << "c5 " << endl << flush;
                            dones++;
                        });
                    });
                });
            });

        });


        
        while (dones < 2)
        {
            usleep(50000);
        }

    }

    cout << "Program end " << endl << flush;

}
