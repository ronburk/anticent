#ifndef LISTENER_H_
#define LISTENER_H_

#include <string>
#include "poll.h"
#include "schedule.h"
using std::string;


class   Listener : Poll::Eventable, Runnable
    {
    string  name;
    int     port = -1;
public:
    Listener(){}
    int    Bind(const string& path);
    int    Bind(const string& nicname, int port, bool Ip6=false);
    ~Listener();
    virtual void Event(int event);
    };

class HttpListener : public Listener
    {
public:
    HttpListener(){}
    };

#endif /*  LISTENER_H_ */
