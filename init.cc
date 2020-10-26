#include "init.h"
#include "eventable.h"
#include "signals.h"
#include <cassert>
#include <memory>
using std::make_unique;
using std::unique_ptr;

unique_ptr<Init> Init::NewInit()
    {
    Eventable::InitEventable();        // initialize event system
    return make_unique<Init>();
    }

Signals* sigs;

Init::Init()
    : Job(nullptr)
    {
    assert(sigs == nullptr);    // we should only ever be called once!
    sigs = new Signals(this);
    Signals::Subscribe(this, 0);
    NewHttpListener("eno1", 8080);
    ++nListeners;
    Constructed();
    }
Init::~Init()
    {
    delete sigs;
    }
void Init::vSignal(int signum)
    {
    fprintf(stderr, "[%s] signum=%d\n",
        __PRETTY_FUNCTION__, signum);
    shuttingDown = true;
    }

void Init::vDeathRequest(Job* child)
    {
    delete child;
    --nListeners;
    fprintf(stderr, "[%s] nListeners now %d\n",
        __PRETTY_FUNCTION__, nListeners);
    if(nListeners <= 0)
        Job::Shutdown();
    }


