#include "signals.h"
#include <signal.h>
#include <sys/signalfd.h>
#include <cstdio>
#include <stdlib.h> // exit()
#include <cassert>
#include <unistd.h>
#include <cerrno>
#include <vector>
using std::vector;

/*

signals are mainly coming in through Init.

crazy idea: an object merely signals its interest to Signals. Signals keeps
a list of these Job pointers (any of which could become invalid!). When a
signal occurs, Signals reschedules all the relevant Job's with priority SIGNAL.
Job knows to call the ::Signal() function instead of ::Run(), and then reschedule
each job at its previous priority. Doesn't work because it might have been blocked,
in which case we would inadvertently cause it to run.

2nd crazy idea: only Jobs that need to be signaled are the ones that don't run.
Probably a time to go for generality rather than micro-efficiency.

So, Signals just keeps vector of everybody who registers to get a signal,
then goes through and runs them when the Signals job runs.

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

vector<Job*>    Subscribers;

void Signals::Subscribe(Job* job, int signum)
    {
    Subscribers.push_back(job);
    }

Signals::Signals(Job* parent)
    : Job(parent)
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
    Eventable::Add(sigFd, EPOLLIN);
    fprintf(stderr, "[%s] SIGINT handler installed on fd %d\n",
        __PRETTY_FUNCTION__, sigFd);
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
        for(auto job: Subscribers)
            job->Signal(info.ssi_signo);
        }
    
    static int foober = 0;
    if(foober++)
        {
        fprintf(stderr, "[%s] double-Ctrl-C exit.\n",
            __PRETTY_FUNCTION__);
        exit(2);
        }
    }

