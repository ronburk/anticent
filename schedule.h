#ifndef JOB_H_
#define JOB_H_

class Runnable;

class JobList
    {
    Runnable*   head;
    Runnable*   tail;
public:
    JobList() : head(nullptr), tail(nullptr) {}
    Runnable* Pop();
    void      Push(Runnable* job);
    Runnable*
    };

class Job
    {
    static JobList      Ready[];
    static JobList      Blocked;
    Job*   next;
    Job*   prev;
    bool        blocked;
    int         priority;
public:
    Job();
    virtual ~Job();
    int     IsBlocked()         { return blocked; }
    int     GetPriority()       { return priority; }
    int     SetPriority(int val){ return priority = val; }
    virtual void Run();
    virtual void Schedule(int priority=-1);
    virtual void Wait();
    };

#endif /*  JOB_H_ */
