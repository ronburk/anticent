#ifndef SIGNALS_H_
#define SIGNALS_H_

#include "job.h"
#include "poll.h"

class   Signals : public Eventable
    {
public:
    Signals();
    virtual void Event(int event);
    };



#endif /* SIGNALS_H_ */
