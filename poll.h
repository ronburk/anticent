#ifndef POLL_H_
#define POLL_H_

#include <sys/epoll.h>

using fd_t = int;  // alias to remind ourselves which int's are really file descriptors

class   Eventable
    {
public:
    static int  Poll(int milliseconds);

    fd_t     fd = -1;     // -1 indicates no longer in epoll's interest list
    Eventable();
    int     Add(fd_t fd, int eventFlags);
    fd_t    Del();
    virtual ~Eventable();
    virtual void Event(int event) = 0;
    };

#endif /* POLL_H_ */
