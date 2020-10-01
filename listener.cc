#include "listener.h"
#include "ipaddr.h"
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

/* Bind() - create a listening socket bound to a specific port #
 *
 * The parameters
 */
void Listener::Listen(const string& nicname, int port, bool Ip6)
    {
    IPaddr  addr(Ip6);
    fd_t listenFd = addr.Bind(nicname, port);
    Add(listenFd, EPOLLIN);
#if 0
    char    portStr[64];
    sprintf(portStr, "%d", port);

    fprintf(stderr, "Call EthAddr\n");
//    auto sockaddr = EthAddrFromName(nicname, Ip6);
    addr.SetAddrFromEth(nicname);
    fd_t listenFd = socket(addr.GetFamily(), SOCK_STREAM|SOCK_NONBLOCK, 0);
    DieIf(listenFd < 0, "socket");

//    ((sockaddr_in*)sockaddr)->sin_port = port;
    addr.SetPort(port);
//    fprintf(stderr, "sockaddr->sa_family=%d, AF_INET=%d, AF_INET6=%d\n", sockaddr->sa_family, AF_INET, AF_INET6);
    int status = bind(listenFd, addr, addr);
//        Ip6?sizeof(struct sockaddr_in6):sizeof(struct sockaddr_in));
    DieIf(status != 0, "bind");
    status = listen(listenFd, 100);
    DieIf(status != 0, "listen");

    return status;
#endif
    }

void Listener::Event(int event)
    {
    fprintf(stderr, "Event!! in listener\n");
    }

Listener::~Listener()
    {
    }

void HttpListener::Event(int event)
    {
    fprintf(stderr, "HttpListener gets event.\n");
    fd_t conn = IPaddr::Accept(fd);
    fprintf(stderr, "HttpListener accepts new connection.\n");
    conn = -1;
    }
