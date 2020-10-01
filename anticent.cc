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
    Poll::Init();
    auto server = make_unique<HttpListener>();
    server->Listen("eno1", 8080);
    Poll::Poll(10*1000);
    V8Main(argc, argv);
//    delete server;
//    Poll::Fini();
    return 0;
}
