#ifndef JOB_H_
#define JOB_H_

enum JobPriority : unsigned short
    {
    PRI_HIGHEST     = 0,
    PRI_HIGH        = 1,
    PRI_LOW         = 2,
    PRI_COUNT       = 3
    };

class   Job2
    {
    JobPriority priority;
    bool        blocked;
    Job2*       previous;
    Job2*       next;
    Job2*       parent;
public:
    Job2(JobPriority priority=PRI_LOW, Job2* parent=nullptr);
    JobPriority SetPriority(JobPriority);
    bool        IsBlocked() { return blocked; }
    void        Block();
    virtual ~Job2();
protected:
    virtual void Run();
    virtual void ChildDied(Job* Child);
    };



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
    friend class JobList;
    static JobList  Ready[];

    Job*        next;
    int         priority;
public:
    enum
        {
        PRI_HIGHEST     = 0,
        PRI_HIGH        = 1,
        PRI_LOW         = 2,
        PRI_COUNT       = 3
        };

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
