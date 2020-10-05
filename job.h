#ifndef JOB_H_
#define JOB_H_

class Job;

class JobList
    {
    Job*    head;
    Job*    tail;
public:
    JobList() : head(nullptr), tail(nullptr) {}
    Job*    Pop();
    void    Push(Job* job);
    Job*    Remove(Job* job);
    };

class Job
    {
    enum
        {
        PRI_SIGNAL      = 0,
        PRI_HIGH        = 1,
        PRI_LOW         = 2,
        PRI_COUNT       = 3
        };
    friend class JobList;
    static JobList  Ready[PRI_COUNT];

    Job*        next;
    int         priority;
public:
    static void     Scheduler();
    static void     Shutdown();

    Job();
    virtual ~Job();
    int     IsBlocked()         { return next == nullptr; }
    int     GetPriority()       { return priority; }
    int     SetPriority(int val){ return priority = val; }
    virtual void Run();
    virtual void Schedule(int priority=-1);
    virtual void Wait();
    };

#endif /*  JOB_H_ */
