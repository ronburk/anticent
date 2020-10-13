#ifndef EVENTABLE_H_
#define EVENTABLE_H_

#include "init.h"
#include <sys/epoll.h>

using fd_t = int;  // alias to remind ourselves which int's are really file descriptors

class Eventable : public Job
    {
    friend void Init::InitEventable();
protected:
    fd_t     fd = -1;     // -1 indicates no longer in epoll's interest list
    virtual const char*  vClassName() { return "Eventable"; }

public:
    static int  Poll(int milliseconds);
    static void PollInit();

    Eventable(Job* parent, short priority=Job::BLOCKED);
    virtual ~Eventable();

    int             Add(fd_t fd, int eventFlags);
    fd_t            Del();
    virtual void    Event(int event) = 0;
    };

#endif  /*  EVENTABLE_H_ */
