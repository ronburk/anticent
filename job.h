#ifndef JOB_H_
#define JOB_H_

class JobList;

enum class JobPriority : unsigned short
    {
    SIGNAL      = 0,
    HIGHEST     = 1,
    HIGH        = 2,
    LOW         = 3,
    BLOCKED     = 4,
    COUNT       = 5
    };

class   Job
    {
    friend class    JobList;
    JobPriority     priority, prevPriority;
    Job*            next;
    Job*            previous;

protected:
    Job*       parent;
    virtual void vSignal(int signum){}
    virtual void vRun(){}
    virtual void vDeathRequest(Job* dyingChild){}
    virtual JobPriority  vBasePriority() { return JobPriority::LOW; }
    virtual const char*  vClassName() { return "Job"; }

public:
    static void Init();
    static void Scheduler();

    Job(Job* parent, JobPriority priority=JobPriority::BLOCKED);
    virtual ~Job();

    JobPriority BasePriority()   { return vBasePriority(); }
    void        Schedule(JobPriority priority=JobPriority::BLOCKED);
    void        Block();
    bool        IsBlocked() { return priority == JobPriority::BLOCKED; }
    const char* ClassName() { return vClassName(); }
    void        Ready();
    void        Run()               { vRun(); }
    void        Signal(int signum)  { vSignal(signum); }
    void        Stop();
    void        DeathRequest(Job* dyingChild) { vDeathRequest(dyingChild); }
    void        Constructed();
    };

#endif /*  JOB_H_ */
