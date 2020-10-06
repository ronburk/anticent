#include "job.h"
#include "poll.h"
#include <cassert>
#include <cstdio>
#include "listener.h"
/* Rewrite:
 *
 * How about if we view the scheduler as "owning" all "runnable" objects?
 * We could cease distinguishing between "eventable" and "runnable".
 *
 * Consider case of an HttpConn Job that is pointed to by an HttpReader Job.
 * We did not make HttpConn a Job because it can get its "running" done when
 * its virtual "event" function gets called.


We could resolve that by creating "Eventable Job". This could have an 'event'
member that accumulates all the I/O events that have occurred since it was
last run. 

But an HttpConn points to an HttpReader Job, because it needs to be able to
unblock that Job when a poll event happens. So, this is a Job pointer that
lies outside of the scheduler, and represents a lifetime problem. We might
encounter a similar problem with a V8 Job that points to a database read
Job.

Though not absolutely essential, we would like the HttpReader Job to be able
to be deleted before the HttpConn Job. For example the client side might have
closed the read side of our connection, meaning the HttpConn and its HttpWriter
may still have work to do, but there is no more reason for the HttpReader to
continue to consume memory.

So, the HttpConn "kinda" owns the HttpReader by virtue of the fact it outlives
it and has a pointer to it that needs to be invalidated.

Idea: could allow Job's of this type to enter a zombie state instead of being
deleted. HttpConn can call HttpReader::Kill(), and that

Or maybe Job's can have parent/child relationship. Suppose only parent Job
can kill a Job. So HttpConn can "kill" HttpReader. But how does HttpReader
let HttpConn know it's dead, assuming things happen in the reverse order?
Maybe it doesn't. Child has parent pointer, child signals parent it is dead.
Parent must zero out its raw pointer to child, child falls off scheduler and
is automatically deleted.

Suppose child is calling on parent for services? Well, if parent is dead,
then it has already killed its children. After a parent kills a child, the
scheduler will not call its Run() function, and will let it get deleted.

NB: ability to kill a Job that is currently scheduled means JobList must
be a container that can support randomly removing a value. Or else, we just
let it sit there until it gets ->Run() and Job::Run() does nothing because
it knows it's been killed.

case: when HttpReader gets EAGAIN, it needs to go into a "blocked" state.
It should only get unblocked by parent Job (HttpConn). So, I think I'm back
to flexibility+overhead of "intrusive linked list". 

Should "blocked" be just another priority, or a separate state?




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
        int runnable = 0;
        for(int iPriority=0; iPriority < PRI_COUNT; ++iPriority)
            {
            while(auto nextJob = Ready[iPriority].Pop())
                {
                nextJob->Run();
                ++runnable;
                }
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
