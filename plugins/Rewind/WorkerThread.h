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


class Worker {
    std::thread* worker;
    std::queue<std::function<void()>> tasks;
    std::mutex mutex;

public:
    Worker()
    {

    }

    ~Worker()
    {
        if (worker != NULL)
        {
            worker->join();
            delete worker;
        }
    }

    template<class F>
    void addTask(F&& f)
    {
        mutex.lock();
        tasks.push(f);
        mutex.unlock();
        if (tasks.size() != 0)
        {
            if (worker != NULL)
            {
                worker->join();
                delete worker;
            }
            worker = new std::thread([=]()
                {
                    while (!tasks.empty()) {
                        mutex.lock();
                        std::function<void()> task = std::move(tasks.front());
                        tasks.pop();
                        mutex.unlock();
                        task();
                    }
                });
        }
    }

};


#endif