#ifndef LISTENER_H_
#define LISTENER_H_

#include <string>
#include "poll.h"
#include "job.h"
using std::string;


class   Listener : public Eventable, public Job
    {
    string  name;
    int     port = -1;
public:
    Listener(){}
    int     Listen(const string& path);
    void    Listen(const string& nicname, int port, bool Ip6=false);
    ~Listener();
    virtual void Event(int event);
    };

class HttpListener : public Listener
    {
    HttpListener(){}
public:
    static void Shutdown();
    static void New(string nicname, int port, bool IP6=false);
    virtual void Event(int event);
    };

#endif /*  LISTENER_H_ */
