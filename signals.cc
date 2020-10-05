#include "signals.h"
#include <signal.h>
#include <sys/signalfd.h>
#include <cstdio>
#include <stdlib.h> // exit()
#include <cassert>
#include <unistd.h>
#include <cerrno>

/* This version is simple: if we get a signal, we shut down.
 * To add functionality for other signals, one would have to change
 * the Event() function so that it actually reads signal events from
 * the fd and acts accordingly.
 * 
 */


Signals::Signals()
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
    
    Job::Shutdown();
    static int foober = 0;
    if(foober++)
        exit(2);
    }

