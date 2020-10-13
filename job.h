#ifndef JOB_H_
#define JOB_H_

class JobList;


class   Job
    {

    friend class    JobList;
    short           priority, prevPriority;
    Job*            next;
    Job*            previous;

protected:
    Job*       parent;
    virtual void    vSignal(int signum){}
    virtual void    vRun(){}
    virtual void    vDeathRequest(Job* dyingChild){}
    virtual short   vBasePriority() { return LOW; }
    virtual const char*  vClassName() { return "Job"; }

public:
enum : unsigned short
    {
    SIGNAL      = 0,
    HIGHEST     = 1,
    HIGH        = 2,
    LOW         = 3,
    BLOCKED     = 4,
    COUNT       = 5
    };

    static void Init();
    static void Scheduler();

    Job(Job* parent, short priority=BLOCKED);
    virtual ~Job();

    short       BasePriority()   { return vBasePriority(); }
    void        Schedule(short priority=BLOCKED);
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
