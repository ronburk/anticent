#include "init.h"
#include "eventable.h"

void Init::NewInit()
    {
    InitEventable();        // initialize event system
    new Init();
    }

Init::Init()
    : Job(nullptr, JobPriority::BLOCKED)
    {
    NewHttpListener("eno1", 8080);
    Constructed();
    }
