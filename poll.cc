#include "poll.h"
#include <unistd.h>
#include <sys/epoll.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "signals.h"

/* DumpEvent() - little debugging code to symbolically dump I/O events.
 */
struct  EventName
    {
    int                    Val;
    const char*     Name;
    };

EventName EventNames[] =
    {
    { EPOLLIN, "EPOLLIN" },
    { EPOLLPRI, "EPOLLPRI" },
    { EPOLLOUT, "EPOLLOUT" },
    { EPOLLOUT, "EPOLLOUT" },
    };

int nEventNames = sizeof(EventNames)/sizeof(EventName);
void    DumpEvent(int event)
    {
    char output[512];

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
    fprintf(stderr, "%s\n", output);
    }


/*

HTTP request either has cookie identifiying session/user, or doesn't, in which case
the request is treated as coming from "world". We need to identify session/user before
performing "URL fetch".

So, HTTP listener callback gets fired and creates a Request. It has no user or session yet.



HTTP request refers to URL. C++ will perform "URL fetch", which
may or may not invoke JavaScript engine.

Three poll lists: HTTP, NORMAL, IDLE

forever()
    {
    repeat do HTTP writes until no work to do
    repeat do DB writes until no work to do
    repeat do DB reads until no work to do
    run V8 to completion
    }

job::
    job->Run()      <-- Run is virtual function
    job->Schedule(event_flags)  <-- Schedule is virtual function

 */


int     nSockets = 0;
fd_t    epollFd;
int Init()
    {
    fd_t result = epoll_create(1);
    if(result < 0)
        {
        perror("epoll_create() failed");
        assert(false);
        }
    fprintf(stderr, "Poll::Init()\n");
    epollFd = result;
    new Signals();

    return result;
    }
int foo = Init(); // ??? so tacky

/* Poll() - poll for I/O events
 */
int  Eventable::Poll(int milliseconds)
    {
    struct epoll_event events[100];
    int     nAwake;

    fprintf(stderr, "epoll_wait(%d)\n", milliseconds);
    nAwake = epoll_wait(epollFd, events, 100, milliseconds);
//        fprintf(stderr, "signalNum = %d\n", signalNum);
    if(nAwake < 0)
        {
        perror("epoll_wait failed");
        assert(false);
        }
    for(int iSocket=0; iSocket < nAwake; ++iSocket)
        {
        DumpEvent(events[iSocket].events);
        static_cast<Eventable*>(events[iSocket].data.ptr)
            ->Event(events[iSocket].events);
        }
    fprintf(stderr, "epoll_wait got %d/%d sockets\n", nAwake, nSockets);
    return nSockets;
    }
void Fini()
    {
    }
// implement class Eventable

Eventable::Eventable() : fd(-1)
    {
    }
int Eventable::Add(int fd, int eventFlags)
    {
    assert(this->fd == -1);  // this object can't have two active descriptors

    fprintf(stderr, "Eventable::Add(fd=%d, 0x%08X)\n", fd, eventFlags);
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
fd_t Eventable::Del()
    {
    assert(this->fd != -1);

    int status = epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, nullptr);
    if(status != 0)
        {
        perror("EPOLL_CTL_DEL failed");
        assert(false);
        }
    else
        {
        this->fd    = -1;
        --nSockets;
        }
    return this->fd;
    }
Eventable::~Eventable()
    {
    if(fd >= 0) // if we have a file descriptor in epoll's interest list
        {
        int status = epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, nullptr);
        if(status != 0)
            {
            perror("EPOLL_CTL_DEL failed");
            assert(false);
            }
        --nSockets;
        }
    close(fd);
    fd = -1;
    }

