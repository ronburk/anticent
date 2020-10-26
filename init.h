#ifndef INIT_H_
#define INIT_H_

#include "job.h"
#include <string>
using std::string;
#include <memory>
using std::unique_ptr;

class Init : public Job
    {
    virtual void            vSignal(int signum);
    bool                    shuttingDown = false;
    int                     nListeners = 0;
protected:
    virtual const char*     vClassName() { return "Init"; }
    virtual void            vDeathRequest(Job*);
public:
    static unique_ptr<Init> NewInit();
    static void             InitEventable();
    void                    NewHttpListener(string nicname, int port, bool IP6=false);

    Init();
   ~Init();
    };

#endif /* INIT_H_ */
