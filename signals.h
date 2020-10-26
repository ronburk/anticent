#ifndef SIGNALS_H_
#define SIGNALS_H_

#include "job.h"
#include "eventable.h"

class   Signals : public Eventable, public Job
    {
public:
    static void Subscribe(Job* job, int signum);

    Signals(Job* parent);
    virtual void Event(int event);
    };

#endif /* SIGNALS_H_ */
