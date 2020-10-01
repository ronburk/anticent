#ifndef LISTENER_H_
#define LISTENER_H_

#include <string>
#include "poll.h"
#include "schedule.h"
using std::string;


class   Listener : public Poll::Eventable, Runnable
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
public:
    HttpListener(){}
    virtual void Event(int event);
    };

#endif /*  LISTENER_H_ */
