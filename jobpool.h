#pragma once

#include <thread>
#include <mutex>
#include <memory>
#include <vector>
#include <queue>
#include <condition_variable>

// Producer/Consumer queue for inter-thread communication
// of chunks of std::vector<float>

class JobQueue {
public:
    struct Data {
    public:
        Data(size_t s=0)
            : _vector(s)
        {
        }
        std::vector<float> _vector;
    };
    typedef std::shared_ptr<Data> dataptr_t;
    JobQueue(int maxsize=10);
    virtual
    ~JobQueue();
    void push(dataptr_t job);
    bool pop(dataptr_t & job);
    size_t size();
    void finish();
private:
    std::condition_variable _cnt;
    std::queue<dataptr_t> _jobs;
    std::mutex _mtx;
    int _maxsize = 0;
    int _count = 0;
    bool _finished = false;
};

// Virtual base class of a Job in the Job pool
class Job {
public:
    Job();
    virtual ~Job();
    virtual bool run() = 0;
};

// A pool of Jobs meant to be executed in different threads
class JobPool {
public:
    typedef std::shared_ptr<Job> jobptr_t;
    virtual ~JobPool();
    void join();
    void addJobs(std::vector<jobptr_t> && queues);
    void start();
private:
    std::vector<std::thread> _threads;
    std::vector<jobptr_t> _jobs;
};
