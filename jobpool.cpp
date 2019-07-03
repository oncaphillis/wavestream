#include "jobpool.h"

JobQueue::JobQueue( int maxsize )
    : _maxsize(maxsize)
{
}

JobQueue::~JobQueue()
{
}

void JobQueue::push(dataptr_t job)
{
    std::unique_lock<decltype (_mtx)> lck(_mtx);

    if(_finished)
    {
        throw std::runtime_error("Illegal push on fiished job queue");
    }

    _cnt.wait(lck,[this](){return _maxsize==0 || _jobs.size()<_maxsize;});
    _jobs.push(job);
    _cnt.notify_all();
}

bool JobQueue::pop(dataptr_t & job)
{
    std::unique_lock<decltype (_mtx)> lck(_mtx);
    _cnt.wait(lck,[this](){return !_jobs.empty() || _finished;});

    if(_jobs.empty())
    {
        return false;
    }

    job = _jobs.front();
    _jobs.pop();
    _cnt.notify_all();

    return true;
}

size_t JobQueue::size()
{
    std::unique_lock<decltype (_mtx)> lck(_mtx);
    return _jobs.size();
}

void JobQueue::finish()
{
    std::unique_lock<decltype (_mtx)> lck(_mtx);
    _finished = true;
    _cnt.notify_all();
}

Job::Job() {
}

Job::~Job() {
}


JobPool::~JobPool()
{
    join();
}

void JobPool::join()
{
    for(auto & t : _threads)
    {
        t.join();
    }
    _threads.resize(0);
}

void JobPool::addJobs(std::vector<jobptr_t> && queues)
{
    for(auto & q : queues)
    {
        _jobs.push_back(q);
    }
}

void JobPool::start() {
    for(auto & j : _jobs)
    {
        _threads.push_back(std::thread([j](){
            while(j->run());
        }));
    }
}
