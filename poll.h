#ifndef POLL_H_
#define POLL_H_

#include <sys/epoll.h>

using fd_t = int;  // alias to remind ourselves which int's are really file descriptors


namespace Poll
    {
    void    Init();
    void    Poll(int milliseconds);

    class   Eventable
        {
    public:
        fd_t     fd = -1;     // -1 indicates no longer in use
        Eventable();
        int Add(fd_t fd, int eventFlags);
        virtual ~Eventable();
        virtual void Event(int event) = 0;
        };
    }


#endif /* POLL_H_ */
