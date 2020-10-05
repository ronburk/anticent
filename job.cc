#include "job.h"
#include "poll.h"
#include <cassert>
#include <cstdio>
#include "listener.h"

/*
 */

//#define NPRIORITIES 2

int     nJobs = 0;

JobList Job::Ready[Job::PRI_COUNT];

void JobList::Push(Job* job)
    {
    assert(job->next == nullptr);
    if(tail == nullptr)
        {
        assert(head == nullptr);
        head = tail = job;
        }
    else
        {
        tail->next  = job;
        tail        = job;
        }
    }

Job* JobList::Pop()
    {
    Job*    result = nullptr;

    if(head)    // if there is anything to pop
        {
        result = head;
        if(head == tail)  // if removing last job from list
            head = tail = nullptr;
        else
            head = head->next;
        result->next = nullptr; // any Job not scheduled must have null 'next'.
        }
    return result;
    }

Job::Job() : priority(0)
    {
    ++nJobs;
    fprintf(stderr, "sizeof(next)=%lu\n", sizeof(next));
    next = nullptr;
    }
Job::~Job()
    {
    assert(nJobs > 0);
    --nJobs;
    }

void Job::Run()
    {
    }
void Job::Schedule(int priority)
    {
    if(priority == -1)
        priority = this->priority;
    assert(priority >= 0);
    assert(priority < PRI_COUNT);
    assert(next == nullptr); // assume not currently scheduled
    Ready[priority].Push(this);
    }
void Job::Wait() {}

static bool FinishUp = false;

void Job::Shutdown()
    {
    HttpListener::Shutdown();
    FinishUp = true;
    }

const int MAX_SHUTDOWN_SECONDS = 15;

/* Schedule() - anticent's main run loop.
 */
void Job::Scheduler()
    {
    int iShutdown = 0;
    for(; nJobs && iShutdown < MAX_SHUTDOWN_SECONDS;)
        {
        for(int iPriority=0; iPriority < PRI_COUNT; ++iPriority)
            {
            int runnable = 0;
            while(auto nextJob = Ready[iPriority].Pop())
                {
                nextJob->Run();
                ++runnable;
                }
            fprintf(stderr, "%d/%d jobs were runnable\n", runnable, nJobs);
            if(runnable == 0)
                {
                int milliseconds = FinishUp ? 1*1000 : 30*1000;
                while(auto nEvents = Eventable::Poll(milliseconds) == 0)
                    if(nJobs <= 0)
                        break;
                if(FinishUp)
                    ++iShutdown;
                }
            }
        }
    }
