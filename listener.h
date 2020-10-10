#ifndef LISTENER_H_
#define LISTENER_H_

#include <string>
#include "eventable.h"
using std::string;


class   Listener : public Eventable
    {
    string  name;
    int     port = -1;
protected:
    virtual const char* vClassName();
public:
    Listener(Job* parent);
    int     Listen(const string& path);
    void    Listen(const string& nicname, int port, bool Ip6=false);
    ~Listener();
    virtual void Event(int event);
    };

#endif /*  LISTENER_H_ */
