#ifndef LISTENER_H_
#define LISTENER_H_

#include "poll.h"
#include "schedule.h"

class   Listener : Poll::Eventable, Runnable
    {
    Listener(int fd);
public:
    static Listener* New(int port);
    ~Listener();
    virtual void Event(int event);
    };

#endif /*  LISTENER_H_ */
