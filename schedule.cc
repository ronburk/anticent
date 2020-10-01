#include "schedule.h"
#include <cassert>

#define NPRIORITIES 2

JobList Runnable::Ready[NPRIORITIES];
JobList Runnable::Blocked;

Runnable::Runnable() : blocked(true), priority(0)
    {
    // always create in blocked state
    Blocked.Push(this);
    }
Runnable::~Runnable()
    {
    
    }

void Runnable::Run()
    {
    }
void Runnable::Schedule(int priority)
    {
    if(priority == -1)
        priority = this->priority;
    assert(priority >= 0);
    assert(priority < NPRIORITIES);

    
    }
void Runnable::Wait() {}
