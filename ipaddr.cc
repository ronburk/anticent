#include "ipaddr.h"
#include <cassert>
#include <ifaddrs.h>
#include <netdb.h>

// little helper function to explain errno and die
void    DieIf(bool failed, const char* name)
    {
    if(failed)
        {
        string msg(name);
        msg     += " failed.";
        perror(msg.c_str());
        assert(false);
        }
    }

void IPaddr::SetPort(int port)
    {
    ip4.sin_port = htons(port);
    }

void IPaddr::SetFromSockaddr(struct sockaddr* other)
    {
    memcpy(&any, other, operator socklen_t());
    }

IPaddr::IPaddr(bool IP6)
    {
    memset(&any, 0, sizeof(any));
    any.sa_family = IP6 ? AF_INET6 : AF_INET;
    }

fd_t IPaddr::Accept(fd_t listenFd)
    {
    fd_t    result;
    IPaddr  newAddr(true);

    socklen_t size = newAddr;
    result = accept4(listenFd, newAddr, &size, SOCK_NONBLOCK);
    // ??? handle EAGAIN, EWOULDBLOCK, ECONNABORTED, EINTR, etc.
    
    DieIf(result < 0, "accept4");
    return result;
    }


void IPaddr::SetAddrFromEth(string ethname)
    {
    struct ifaddrs*     listhead;
    int status = getifaddrs(&listhead);
    DieIf(status != 0, "getifaddrs");

    bool found = false;
    for(auto list=listhead; list; list = list->ifa_next)
        if(list->ifa_name == ethname && list->ifa_addr) // ifa_addr sometimes null...
            if(list->ifa_addr->sa_family==GetFamily())
                {
                SetFromSockaddr(list->ifa_addr);
                found           = true;
                }
    freeifaddrs(listhead);
    if(!found)
        {
        fprintf(stderr, "SetAddrFromEth() could not find interface '%s'\n",
            ethname.c_str());
        assert(false);
        }
    }


string mygetname(const struct sockaddr* addr, int flags)
    {
    char        output[256];
    socklen_t socklen;
    switch(addr->sa_family)
        {
        case AF_INET6   : socklen = sizeof(struct sockaddr_in6);    break;
        case AF_INET    : socklen = sizeof(struct sockaddr_in);     break;
        default: assert(false);
        }

    int     status = getnameinfo(addr, socklen, output, sizeof(output)-1, nullptr, 0, flags);
    const char* errtext = nullptr;
    switch(status)
        {
        case    EAI_AGAIN:
            errtext = "EAI_AGEIN: name could not be resolved; try again later";
            break;
        case    EAI_BADFLAGS:
            errtext = "EAI_BADFLAGS: invalid 'flags' argument";
            break;
        case    EAI_FAIL:
            errtext = "EAI_FAIL: nonrecoverable error occurred";
            break;
        case    EAI_FAMILY:
            errtext = "EAI_FAMILY: address family unknown or address length wrong for family";
            break;
        case    EAI_MEMORY:
            errtext = "EAI_MEMORY: out of memory";
            break;
        case    EAI_NONAME:
            errtext = "EAI_NONAME: NI_NAMEREQD is set and host's name can't be located, or neither host/service name was requested";
            break;
        case    EAI_OVERFLOW:
            errtext = "EAI_OVERFLOW: output buffer too small";
            break;
        case    EAI_SYSTEM:
            DieIf(true, "getnameinfo");
            break;
        }
    if(errtext)
        {
        fprintf(stderr, "getnameinfo() failed: '%s'\n", errtext);
        assert(false);
        }
    return output;
    }

fd_t IPaddr::Bind(const string& nicname, int port)
    {
    int status;
    SetAddrFromEth(nicname);
    fd_t listenFd = socket(GetFamily(), SOCK_STREAM|SOCK_NONBLOCK, 0);
    DieIf(listenFd < 0, "socket");
    // next two lines assume HTTP listener
    int   flag = 1;
    status = setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    DieIf(status != 0, "setsockopt(SO_REUSEADDR)");
    SetPort(port);
    status = bind(listenFd, *this, *this); // conv operators know what to do
    DieIf(status != 0, "bind");
    status = listen(listenFd, 100);
    DieIf(status != 0, "listen");

    return listenFd;
    }


#if 0
struct sockaddr* EthAddrFromName(const string& name, bool Ip6)
    {
    static struct sockaddr ImGoingToHell;
    struct ifaddrs*     listhead;
    int status = getifaddrs(&listhead);
    DieIf(status != 0, "getifaddrs");

    bool found = false;
    for(auto list=listhead; list; list = list->ifa_next)
        if(list->ifa_name == name && list->ifa_addr)
            {
//            char    dumpstr[256] = "";
            int     family = list->ifa_addr->sa_family;
            if(family==AF_INET6 || family==AF_INET)
                {
                auto addrstr = mygetname(list->ifa_addr, NI_NUMERICHOST);
                fprintf(stderr, "Interface '%s', IP%s addr='%s'\n", name.c_str(),
                    (family==AF_INET6)?"6":"", addrstr.c_str());
                if((Ip6 && (family==AF_INET6)) || (family == AF_INET))
                    {
                    ImGoingToHell   = *list->ifa_addr;
                    found           = true;
                    }
                }
            else if(family == AF_PACKET) // sometimes called AF_LINK?
                ;   // could print MAC address I guess
            else    // else let's notice if the possibilities are different than we imagine
                {
                fprintf(stderr, "what is family %d on %s?\n", list->ifa_addr->sa_family, name.c_str());
                assert(false);
                }
            }
    freeifaddrs(listhead);
    if(found)
        return &ImGoingToHell;
    fprintf(stderr, "EthAddrFromName() could not find interface '%s'\n",
        name.c_str());
    assert(false);
    }



#endif
