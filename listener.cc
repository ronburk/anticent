#include "listener.h"
#include "ipaddr.h"
#include "http.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <cstring>   // memset
#include <cstdio>
#include <cassert>
#include <unistd.h>

Listener::Listener(Job* parent)
    : Job(parent)
    {
    Constructed();
    }


void Listener::Listen(const string& nicname, int port, bool Ip6)
    {
    fprintf(stderr, "Listener::Listen('%s',%d)\n", nicname.c_str(), port);
    IPaddr  addr(Ip6);
    fd_t listenFd = addr.Bind(nicname, port);
    Add(listenFd, EPOLLIN);
    }

void Listener::Event(int event)
    {
    fprintf(stderr, "Event!! in listener\n");
    }

Listener::~Listener()
    {
    }

const char* Listener::vClassName()
    {
    return "Listener";
    }

