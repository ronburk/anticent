#include "process.h"
using std::make_shared;

Process::Process(int uid, int layer, std::string cwd)
    : uid(uid), layer(layer), cwd(cwd)
    {
    }

shared_ptr<Process> Process::New(int uid, int layer, string cwd)
    {
    return make_shared<Process>(uid, layer, cwd);
    }
