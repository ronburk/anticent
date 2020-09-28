#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
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
    auto server = Listener::New(8080);
    Poll::Poll();
    V8Main(argc, argv);
    delete server;
//    Poll::Fini();
    return 0;
}
