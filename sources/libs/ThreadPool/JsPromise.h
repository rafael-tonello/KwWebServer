//File version 1

#ifndef __JsPromise_H_
#define __JsPromise_H_

#include <iostream>
#include <functional>
#include <vector>
#include <mutex>
#include <chrono>
#include "ThreadPool.h"

using namespace std;

class Promise{
    
    //this vector contaisn a list of function that must be executed one by one
    vector<function<void(void)>> thens;

    //this vector is used to a observation system.
    bool runIsRunning = false;
    timed_mutex thenLocker;
    
    public:
        string promiseName = "";

        static ThreadPool *tasker;
        static bool initDone;
        static void initLib(ThreadPool *nTasker = NULL);
        static mutex __garbageLocker;
        static vector<Promise*> __garbage;

        Promise(function<void()> mainFunction, string debugName = "");
        Promise* then(function<void()> callback);

        void finalize();

    private:    
        bool stopped = false;
        void run();
        void debug(string msg);
    };

/*

    Promise prom1(somefunc).then(somefunc2);
    Promise prom2(somefunc3);
    
    Promise.waitAllProm(prom1, prom2).then(someFunc4);

*/

#endif
