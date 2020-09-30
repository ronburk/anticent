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
    char* foo = new char[100];
    delete[] foo;
    foo[2] = 'a';
    foo[3] = 'b';


#if 0
    char* foo = (char*)malloc(16);
    foo = 0;
    foo = (char*)malloc(999);
    free(foo);
    foo[3] = 'a';
#endif
    Poll::Init();
    auto server = make_unique<HttpListener>();
    // HttpListener server(8080)
    if(server->Bind("eno1", 8080))
        {
        
        }
    Poll::Poll(10*1000);
    V8Main(argc, argv);
//    delete server;
//    Poll::Fini();
    return 0;
}
