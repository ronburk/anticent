#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
using std::make_unique;
#include <cassert>

#include "anticent.h"
#include "listener.h"
#include "poll.h"




int main(int argc, char* argv[])
    {
    HttpListener::New("eno1", 8080);
    V8Main(argc, argv);
    Job::Scheduler();
    return 0;
    }
