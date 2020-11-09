#ifndef JOB_H_
#define JOB_H_

#include "dlist.h"

class RunCallback
    {
public:
    virtual void operator()() = 0;
    };



class Job;

class   Job : public DLink
    {
    friend class        JobList;
    short               priority, prevPriority;
//    Job*                next;
//    Job*                previous;
protected:
    Job*                parent;
    virtual void        vSignal(int signum){}
    virtual void        vRun(){}
    virtual void        vDeathRequest(Job* dyingChild){}
    virtual short       vBasePriority() { return LOW; }
    virtual const char* vClassName() { return "Job"; }
public:
    enum : unsigned short
        {
        ROOT        = 0,
        HIGHEST     = 1,
        HIGH        = 2,
        LOW         = 3,
        BLOCKED     = 4,
        COUNT       = 5
        };

    static void Scheduler();
    static void Shutdown();

    Job(Job* parent);
    virtual ~Job();

//    short       BasePriority()   { return vBasePriority(); }
    void        SetPriority(short priority);
    void        Schedule(short priority=BLOCKED, short prevPriority=LOW);
    void        Unschedule();
    void        Block();
    bool        IsBlocked() { return priority == BLOCKED; }
    const char* ClassName() { return vClassName(); }
    void        Ready();
    void        Run()               { vRun(); }
    void        Signal(int signum)  { vSignal(signum); }
    void        DeathRequest();
    void        Constructed();
    };

#endif /*  JOB_H_ */
