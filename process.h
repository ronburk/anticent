#ifndef PROCESS_H_
#define PROCESS_H_

#ifndef COMMON_H_
#include "common.h"
#endif


class Process
    {
public:
    Process(int uid, int layer, std::string cwd, int parentProcid=0);
    virtual ~Process(){}
    int         procid;  // ID of this process
    int         uid;     // ID of user owning this process
//    int     gid;   // let's not do groups just yet
    int         layer;   // ID of resource identifying active layer (0 for base layer)
    string      cwd;
    int         parentProcID;
    static shared_ptr<Process> New(int uid, int layer, string cwd);
protected:
    };

#endif /* PROCESS_H_ */
