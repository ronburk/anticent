#ifndef INIT_H_
#define INIT_H_

#include "job.h"
#include <string>
using std::string;

class Init : public Job
    {
protected:
    virtual const char*  vClassName() { return "Init"; }

public:
    static void NewInit();
    static void InitEventable();
    void        NewHttpListener(string nicname, int port, bool IP6=false);

    Init();
    };

#endif /* INIT_H_ */
