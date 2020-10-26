#ifndef EVENTABLE_H_
#define EVENTABLE_H_

//???#include "init.h"
#include <sys/epoll.h>

using fd_t = int;  // alias to remind ourselves which int's are really file descriptors

class Eventable
    {
protected:
    fd_t     fd = -1;     // -1 indicates no longer in epoll's interest list
    virtual const char*  vClassName() { return "Eventable"; }

public:
    static int  Poll(int milliseconds);
    static void InitEventable();

    Eventable();
    virtual ~Eventable();

    int             Add(fd_t fd, int eventFlags);
    fd_t            Ignore();
    void            Close();
    virtual void    Event(int event) = 0;
    };

#endif  /*  EVENTABLE_H_ */
