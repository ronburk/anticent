#include "signals.h"
#include <signal.h>
#include <sys/signalfd.h>
#include <cstdio>
#include <stdlib.h> // exit()
#include <cassert>
#include <unistd.h>
#include <cerrno>

/*

signals are mainly coming in through Init.

crazy idea: an object merely signals its interest to Signals. Signals keeps
a list of these Job pointers (any of which could become invalid!). When a
signal occurs, Signals reschedules all the relevant Job's with priority SIGNAL.
Job knows to call the ::Signal() function instead of ::Run(), and then reschedule
each job at its previous priority.

  Any particular type of Job (descendant) might want to receive any
particular type of signal. Signals must have

1) a pointer to the Job object that wants to get signaled
2) a list of which signals the Job object is interested in

The Job object must have

1) a function it wants called when a signal occurs.
2) a way to tell Signals it's dying, so its pointer is invalid

It is a class that is associated with a signal, not an object.
What we want is this: if a class can be signaled, it should
automatically deregister its signal handler when it destructs.

 */

/* This version is simple: if we get a signal, we shut down.
 * To add functionality for other signals, one would have to change
 * the Event() function so that it actually reads signal events from
 * the fd and acts accordingly.
 * 
 */


Signals::Signals(Job* parent)
    : Eventable(parent)
    {
    sigset_t    signalMask;

    sigemptyset(&signalMask);
    sigaddset(&signalMask, SIGINT);
    sigprocmask(SIG_BLOCK, &signalMask, nullptr);

    fd_t sigFd = signalfd(-1, &signalMask, SFD_NONBLOCK);
    if(sigFd == -1)
        {
        perror("signalfd");
        assert(false);
        }
    fprintf(stderr, "SIGINT handler installed, I hope.\n");
    Eventable::Add(sigFd, EPOLLIN);
    }
void Signals::Event(int event)
    {
    fprintf(stderr, "Got signal event!\n");
    fflush(stderr);
    assert(event&EPOLLIN);
    signalfd_siginfo info;
    for(;;)
        {
        ssize_t nBytes = read(Eventable::fd, &info, sizeof(info));
        if(nBytes < 0)
            {
            if(errno == EAGAIN)
                break;
            else
                {
                perror("Signals::Event call to read");
                assert(false);
                }
            }
        assert(nBytes == sizeof(info));
        }
    
    static int foober = 0;
    if(foober++)
        exit(2);
    }

