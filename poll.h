#ifndef POLL_H_
#define POLL_H_

#include <sys/epoll.h>

/*
Add(myfd, mydataptr, mycallback)
mycallback(mydataptr, myfd, eventmask)


 */

namespace Poll
    {
    void    Init();
    void    Poll();

    class   Eventable
        {
    public:
        int fd;
        Eventable(int fd, int eventFlags);
        virtual ~Eventable();
        virtual void Event(int event) = 0;
        };
    }


#if 0
class EPollSource
    {
public:
    EPollSource(int fd);
    
    };
#endif

#endif /* POLL_H_ */
