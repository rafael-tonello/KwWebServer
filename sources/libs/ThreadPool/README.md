# ThreadPool
A ThreadPool library for c++



#ThreadPool examples

```c++
//a very simple task

ThreadPool th;
th.enqueue([&](){
  //do domething
});
```


```c++
//Waiting for a task and get its result

ThreadPool th;
auto taskFuture = th.enqueue([&](){
  //do domething
  //return something
});

auto ret = taskFuture.get();
```


JsPromise example

```c++
ThreadPool tp;
JsPromise::initLib(&tp);

auto prom = new JsPromise([](){
  //do something
});

prom.then([](){
  //do more things
}).then([](){
  //and do more things
});

prom.finalize();


```
