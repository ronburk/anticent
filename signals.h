#ifndef SIGNALS_H_
#define SIGNALS_H_

#include "eventable.h"

class   Signals : public Eventable
    {
public:
    static void Subscribe(Job* job, int signum);
    static void Unsubscribe(Job* job);

    Signals(Job* parent);
    virtual void Event(int event);
    };

#endif /* SIGNALS_H_ */
