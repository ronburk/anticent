#include "eventable.h"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <unistd.h> // close()

/* DumpEvent() - little debugging code to symbolically dump I/O events.
 */
struct  EventName
    {
    int                    Val;
    const char*     Name;
    };

EventName EventNames[] =
    {
    { EPOLLIN,      "EPOLLIN"       },
    { EPOLLPRI,     "EPOLLPRI"      },
    { EPOLLOUT,     "EPOLLOUT"      },
    { EPOLLRDHUP,   "EPOLLRDHUP"    },
    { EPOLLHUP,     "EPOLLHUP"      },
    { EPOLLERR,     "EPOLLERR"      },
    };

int nEventNames = sizeof(EventNames)/sizeof(EventName);
const char*    DumpEvent(int event)
    {
    static char output[64];

    output[0] = '\0';
    for(int iVal = 1; iVal; iVal<<=1)
        {
        int  iEvent;
        for(iEvent = 0; iEvent < nEventNames; ++iEvent)
            {
            if((EventNames[iEvent].Val&event) == iVal)
                {
                if(output[0] != '\0')
                    strcat(output, "|");
                strcat(output, EventNames[iEvent].Name);
                break;
                }
            }
        if(iEvent >= nEventNames && (event&iVal) != 0)
            {
            if(output[0] != '\0')
                strcat(output, "|");
            sprintf(output+strlen(output), "0x%08X", iVal);
            }
        }
    return output;
    }



fd_t    epollFd;    // file descriptor for epoll interest group
int     nSockets;
void Eventable::InitEventable()
    {
    fprintf(stderr, "InitEventable()\n");
    assert(epollFd == 0); // should only get called once!

    epollFd     = epoll_create(1);
    if(epollFd < 0)
        {
        perror("epoll_create() failed");
        assert(false);
        }
    }


/* Poll() - poll for I/O events
 */
int  Eventable::Poll(int milliseconds)
    {
    struct epoll_event  events[100];
    int                 nAwake;
    bool                needNl = true;

    fprintf(stderr, "epoll_wait(%d) ", milliseconds);
    nAwake = epoll_wait(epollFd, events, 100, milliseconds);
    if(nAwake < 0)
        {
        perror("epoll_wait failed");
        assert(false);
        }
    for(int iSocket=0; iSocket < nAwake; ++iSocket)
        {
        auto This = static_cast<Eventable*>(events[iSocket].data.ptr);
        auto eventFlags = events[iSocket].events;
        if(needNl)
            {
            fprintf(stderr, "\n");
            needNl = false;
            }
        fprintf(stderr, "event on socket [%d] -> %s\n",
            This->fd, DumpEvent(eventFlags));

        This->Event(eventFlags);
        }
    fprintf(stderr, "got %d/%d sockets\n", nAwake, nSockets);
    return nSockets;
    }


Eventable::Eventable()
    {
    }
int Eventable::Add(int fd, int eventFlags)
    {
    assert(this->fd == -1);  // this object can't have two active descriptors

    fprintf(stderr, "Eventable::Add(fd=%d, %s)\n",
        fd, DumpEvent(eventFlags));
    // we assume everything is edge-triggered
    struct epoll_event event = {eventFlags|EPOLLET, {this}};
    int status = epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &event);
    if(status != 0)
        {
        perror("EPOLL_CTL_ADD failed");
        assert(false);
        }
    else
        {
        this->fd    = fd;   // destructor is going to need this.
        ++nSockets;
        }
    return status;
    }

void Eventable::Close()
    {
    close(fd);
    fd  = -1;
    --nSockets;
    }


fd_t Eventable::Ignore()
    {
    assert(fd != -1);

    int status = epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, nullptr);
    if(status != 0)
        {
        perror("EPOLL_CTL_DEL failed");
        assert(false);
        }
    --nSockets;
    return fd;
    }
Eventable::~Eventable()
    {
    fprintf(stderr, "Eventable::~Eventable() fd=%d\n", fd);
    if(fd >= 0) // if we have a file descriptor in epoll's interest list
        Close();
    }
