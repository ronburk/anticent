#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
using std::make_unique;
#include <cassert>

#include "anticent.h"
#include "init.h"



int main(int argc, char* argv[])
    {
//    HttpListener::New("eno1", 8080);
    Init::NewInit();
    V8Main(argc, argv);
    Job::Scheduler();
    fprintf(stderr, "normal exit.\n");
    return 0;
    }
