#include "listener.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <cstring>   // memset
#include <cstdio>
#include <cassert>

Listener* Listener::New(int port)
    {
    struct addrinfo*   servinfo;  

    int                      status;
    struct addrinfo  hints;
    char                    portStr[64];

    memset(&hints, 0, sizeof hints); // make sure the struct is empty
    hints.ai_family        = AF_UNSPEC;     // don't care IPv4 or IPv6
    hints.ai_socktype   = SOCK_STREAM; // TCP stream sockets
    hints.ai_flags          = AI_PASSIVE;     // fill in my IP for me

    sprintf(portStr, "%d", port);
    status = getaddrinfo(nullptr, portStr, &hints, &servinfo);
    if(status != 0)
        {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        assert(false);
        }
    // servinfo now points to a linked list of 1 or more struct addrinfos

    int listenFd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if(listenFd < 0)
        {
        perror("socket() failed");
        assert(false);
        }
        
#if 0
    struct epoll_event item = {EPOLLIN, {nullptr}};
    status = epoll_ctl(epollFd, EPOLL_CTL_ADD, listenFd, &item);
    if(status != 0)
        {
        perror("epoll_ctl failed");
        assert(false);
        }
#endif

    status = bind(listenFd, servinfo->ai_addr, servinfo->ai_addrlen);
    if(status != 0)
        {
        perror("bind failed");
        assert(false);
        }
    freeaddrinfo(servinfo); // free the linked-list
    return new Listener(listenFd);
    
    }



Listener::Listener(int fd) : Poll::Eventable(fd, EPOLLIN)
    {
    }

void Listener::Event(int event)
    {
    fprintf(stderr, "Event!! in listener\n");
    }

Listener::~Listener()
    {
    }
