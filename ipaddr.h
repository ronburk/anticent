#ifndef IPADDR_H_
#define IPADDR_H_

/* ipaddr.h - define TCP sockets
 *
 * Main point is to hide the tedious distinguishing between
 * IPv4 and IPv6 socket structures. Note that IPaddr only holds
 * a socket structure, not a file descriptor. Don't want to force
 * keeping around this info for 10,000 open connections, especially
 * since it can be retrieved via the file descriptor by getsockname().
 */

#include <string>
using std::string;

#include <sys/socket.h>
#include <arpa/inet.h>

using fd_t  = int;

class   IPaddr
    {
    union
        {
        struct sockaddr     any;
        struct sockaddr_in  ip4;
        struct sockaddr_in6 ip6;
        };
public:
    static fd_t Accept(fd_t listenFd);
    IPaddr(bool IP6);
    bool IsIP6() { return any.sa_family == AF_INET6; }
    int  GetFamily() { return any.sa_family; }
    void SetFromSockaddr(struct sockaddr* other);
    void SetPort(int port);
    operator socklen_t()
        {
        return any.sa_family == AF_INET ?
            sizeof(struct sockaddr_in)
            : sizeof(struct sockaddr_in6);
        }
    operator struct sockaddr*() { return &any;    }
    void SetAddrFromEth(string ethname);
//    fd_t Accept(fd_t listenFd);
    fd_t Bind(const string& nicname, int port);
    };

#endif /*  IPADDR_H_ */
