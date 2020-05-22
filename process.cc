/* process.cc - implement Process, a context in which a given V8 Task runs.

Every V8 Task belongs to (holds a shared_ptr to) a Process. A Process is analogous
to a *nix process, in that it provides context for code when it is running.


 */

#include "process.h"
using std::make_shared;

int                         nextProcId = 0;
vector<shared_ptr<Process>> procList(10);


Process::Process(int uid, int layer, std::string cwd, int parentProcID)
    : uid(uid), layer(layer), cwd(cwd), parentProcID(parentProcID)
    {
    procid = nextProcId++;   // assign this process the next available process ID
    // ???TODO handle wraparound of procid
    assert(procid < 9999);
    }

/* New() - return shared pointer to new Process.
 */
shared_ptr<Process> Process::New(int uid, int layer, string cwd)
    {
    auto    Result = make_shared<Process>(uid, layer, cwd);

    procList.push_back(Result);
    return Result;
    }
