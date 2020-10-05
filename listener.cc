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

/* Bind() - create a listening socket bound to a specific port #
 *
 * The parameters
 */
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

void HttpListener::Event(int event)
    {
    fprintf(stderr, "HttpListener gets event.\n");
    fd_t conn = IPaddr::Accept(fd);
    fprintf(stderr, "HttpListener accepts new connection.\n");
    HttpConn::New(conn);
    }

static HttpListener* This;

/* Shutdown() - trivial version
 *
 * Take ourselves out of the epoll interest list, and close our listening socket.
 * Not sure it needs to be any fancier than this. I suppose we could try to empty
 * out connections already in the accept queue if, for example, we knew we were
 * being fronted by a load balancer that has shut off the spigot of new connections.
 */
void HttpListener::Shutdown()
    {
    fd_t socket = This->Del();
    close(socket);
    }

void HttpListener::New(string nicname, int port, bool IP6)
    {
    assert(This == nullptr); // initial design allows for exactly one HTTP listener.
    This = new HttpListener();
    This->Listen(nicname, port, IP6);
    This->Schedule(0);
    }
