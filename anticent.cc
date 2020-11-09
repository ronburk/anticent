#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
using std::make_unique;
#include <cassert>

#include "anticent.h"
#include "init.h"
#include "job.h"

#include <sanitizer/lsan_interface.h>

char* Leak()
    {
    char * waste = new char[160*1024];

    if(waste)
        {
        if(waste[0] == 0)
            ++waste[0];
        else
            --waste[0];
        }
    fprintf(stderr, "Leak()\n");
    return waste;
    }

int main(int argc, char* argv[])
    {
//    HttpListener::New("eno1", 8080);
    auto init = Init::NewInit();
    V8Main(argc, argv);
    Job::Scheduler();
    __lsan_do_leak_check();
    fprintf(stderr, "normal exit. \n");
    __lsan_do_recoverable_leak_check();
    return 0;
    }
