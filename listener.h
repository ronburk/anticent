#ifndef LISTENER_H_
#define LISTENER_H_

#include <string>
using std::string;
#include "eventable.h"
#include "job.h"

class   Listener : public Eventable, public Job
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
