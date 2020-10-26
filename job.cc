#include "job.h"
#include <cassert>
#include <cstdio>
#include "listener.h"
#include <unistd.h> // sleep

/*

-- Who is responsible for creating a Job?
Parent

-- Who is responsible for putting a Job into the scheduler?
Parent

-- Who is responsible for removing a Job from the scheduler?
self destructor

-- Who is responsible for destroying a Job?

Exceptions:
a) HttpRequest starts out in non-scheduler JobList owned by HttpConn
b) HttpConn may effectively die before its children, as requests keep processing
c) HttpReader might kill itself, might be killed by parent HttpConn

if we force all these to be events, then we can more easily
track who did what and that they all got done (e.g., in self destructor,
assert(state == DEAD)).

Does parent always want to be notified of child death?

 */


const char* Priname(short pri)
    {
    const char* result = "Can't happen.";
    switch(pri)
        {
        case    Job::ROOT  :
            result = "ROOT";  break;
        case    Job::HIGHEST :
            result = "HIGHEST"; break;
        case    Job::HIGH    :
            result = "HIGH";    break;
        case    Job::LOW     :
            result = "LOW";     break;
        case    Job::BLOCKED :
            result = "BLOCKED"; break;
        case    Job::COUNT   :
            result = "COUNT";   break;
        }
    return result;
    }

#if 0
bool    JobList::Contains(Job* This)
    {
    bool        result  = false;
    if(head)
        {
        Job*        rover   = head;
        do  {
            fprintf(stderr, "----Contains %p\n", rover);
            if(rover == This)
                result = true;
            else
                rover   = rover->next;
            } while(!result && rover != head);
        }
    
    return result;
    }
#endif

#if 0
Job* JobList::Pop()
    {
    Job*    result = nullptr;
    if(count)
        result = Remove(head);
    return result;
    }

Job* JobList::Remove(Job* Job)
    {
    fprintf(stderr, "JobList[%s]::Remove %p\n", Priname(Job->priority), Job);
    assert(count > 0);
    assert(Contains(Job));
    assert(Job->next != nullptr && Job->previous != nullptr);
    if(Job->next == Job)  // if last Job in circular queue
        {
        assert(count == 1);
        head    = nullptr;
        }
    else
        {
        if(head == Job)             // if this is head Job in queue
            head    = Job->next;
        Job->next->previous = Job->previous;
        Job->previous->next = Job->next;
        }
    Job->next = Job->previous = nullptr;
    --count;
    return Job;
    }

void JobList::Push(Job* newTail)
    {
//    fprintf(stderr, "Push %p\n", newTail);
    assert(newTail->next == nullptr); // should not be on any list
    assert(newTail->previous == nullptr);
    if(head == nullptr)
        {
        assert(count == 0);
        head = newTail;
        newTail->next = newTail->previous = newTail;
        }
    else
        {
        auto oldTail            = head->previous;
        newTail->next           = head;
        newTail->previous       = oldTail;
        oldTail->next           = newTail;
        head->previous          = newTail;
        }
    ++count;
    assert(Contains(newTail));
    }
#endif
#if 0
Job* JobList::Pop()
    {
    Job*    result = nullptr;

    if(head)    // if there is anything to pop
        {
        result = head;
        if(head == tail)  // if removing last job from list
            head = tail = nullptr;
        else
            {
            head = head->next;
            tail->next  = head;
            }
        result->next    = nullptr; // any Job not scheduled must have null 'next'.
        result->previous= nullptr;
        }
    return result;
    }
#endif

const int MAX_SHUTDOWN_SECONDS = 5;
const int MAX_PRIORITY_LEVELS  = Job::COUNT;
int     nJobs = 0;
//JobList Jobs[MAX_PRIORITY_LEVELS];
DList Jobs[MAX_PRIORITY_LEVELS];
bool    shutItDown = false;

void Job::Shutdown()
    {
    shutItDown = true;
    }

Job::Job(Job* parent)
    : priority(-1), prevPriority(-1), parent(parent)
    {
    ++nJobs;
    }

Job::~Job()
    {
    fprintf(stderr, "%s (%p) destructed at priority %d/%d\n",
        ClassName(), this, priority, prevPriority);
    assert(nJobs > 0);
    --nJobs;
    }

void Job::SetPriority(short priority)
    {
    prevPriority    = priority;
    if(!IsBlocked())
        {
        
        }
    }

void Job::Block()
    {
    if(!IsBlocked())
        {
        prevPriority = priority;
        Jobs[priority].Remove(this);
        Jobs[priority=BLOCKED].Push(this);
        }
    else
        fprintf(stderr, "%s already blocked!\n", ClassName());
    }
void Job::Ready()
    {
    if(IsBlocked())
        {
        Jobs[priority].Remove(this);
        Jobs[prevPriority].Push(this);
        priority    = prevPriority;
        }
    else
        fprintf(stderr, "%s already ready!\n", ClassName());
    }

void Job::Constructed()
    {
    fprintf(stderr, "%s (%p) constructed at priority %d/%d\n",
        ClassName(), this, priority, prevPriority);
    }

/* job leaves the scheduler */
void Job::Unschedule()
    {
    Jobs[priority].Remove(this);
    assert(this->next == nullptr);
    assert(this->previous == nullptr);
    }

/* job enters the scheduler */
void Job::Schedule(short newPriority, short newPrevPriority)
    {
    assert(previous == nullptr);
    assert(next == nullptr);

    fprintf(stderr, "Job::Schedule '%s'(%s->%s)\n",
        ClassName(), Priname(priority), Priname(newPriority));

    Jobs[newPriority].Push(this);
    this->priority      = newPriority;
    this->prevPriority  = newPrevPriority;
    }



/* Scheduler() - anticent's main run loop.
 */
void Job::Scheduler()
    {
    int iShutdown = 0;
    for(; nJobs && iShutdown < MAX_SHUTDOWN_SECONDS;)
        {
        int runnable    = 0;
        for(int iPriority=0; iPriority < MAX_PRIORITY_LEVELS; ++iPriority)
            {
            auto    nRunnable = Jobs[iPriority].Count();
            auto    iJob      = (Job*)Jobs[iPriority].Head();
//            fprintf(stderr, "JobList[%d=%8.8s].Count() = %d\n",
//                iPriority, Priname(iPriority), nRunnable);
            if(iPriority < MAX_PRIORITY_LEVELS-1)
                {
                for(int iRunnable=0; iRunnable < nRunnable; ++iRunnable)
                    {
                    auto next = iJob->next;  // save it before it might change
                    fprintf(stderr, "%s->Run()\n",
                        iJob->ClassName());
                    assert(iJob->priority == iPriority);
                    iJob->Run();
                    iJob    = (Job*)next;
                    }
                runnable += nRunnable;
                }
            }
        fprintf(stderr, "%d/%d jobs were runnable (%d blocked)\n",
            runnable, nJobs, Jobs[BLOCKED].Count());
//        assert(nJobs == runnable + Jobs[Job::BLOCKED].count);
        if(runnable == 0)
            {
            int milliseconds = shutItDown ? 1*1000 : 30*1000;
            while(auto nEvents = Eventable::Poll(milliseconds) == 0)
                if(nJobs <= 0)
                    break;
            if(shutItDown)
                ++iShutdown;
            }
        }
    }

void Job::DeathRequest()
    {
    if(parent)
        parent->vDeathRequest(this);
    }



#if 0
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

Should "blocked" be just another priority, or a separate state? The difference
is that if "blocked" is a separate piece of state, then nobody has to decide
what priority the Job gets when it is unblocked. But having "prevPriority"
costs about the same and makes things a bit more uniform. No separate block/unblock
functions/data, IsBlocked is just GetPriority() == PRI_BLOCKED

Startup initialization can be done with special Init Job.

So, if Job has a parent, then the parent "owns" the Job and is responsible
for deleting it. If Job has no parent, then the schedule "owns" the Job and
is responsible for deleting it.

But suppose that HttpListener owns every HttpConn it creates. It wants to keep
a count of how many connections are alive, but doesn't want to retain a list
of pointers to all open connections. OK, child does not call delete, but calls
parent->RequestDeath(), passing pointer to itself. Parent can adjust count and
then delete the child.


 */

//#define NPRIORITIES 2

int     nJobs = 0;

JobList Job::Jobs[Job::PRI_COUNT];

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

Job::Job(Job* parent, JobPriority priority)
    : priority(priority), next(nullptr), previous(nullptr)
    {
    ++nJobs;
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
    Jobs[priority].Push(this);
    }
void Job::Wait() {}

static bool FinishUp = false;

void Job::Shutdown()
    {
    HttpListener::Shutdown();
    FinishUp = true;
    }

const int MAX_SHUTDOWN_SECONDS = 15;

/* Scheduler() - anticent's main run loop.
 */
void Job::Scheduler()
    {
    int iShutdown = 0;
    for(; nJobs && iShutdown < MAX_SHUTDOWN_SECONDS;)
        {
        int runnable = 0;
        for(int iPriority=0; iPriority < PRI_COUNT; ++iPriority)
            {
            while(auto nextJob = Jobs[iPriority].Pop())
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
        else
            sleep(1);
        }
    }
#endif
