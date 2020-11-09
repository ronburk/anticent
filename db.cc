#include "db.h"
#include <cassert>

/*

We need to get information from the database, but be able to let the
database decide whether or not that information can be obtained
synchronously or not. Those functions that may block take the following
rough form:

    bool  DBFunction(Job* Caller, SomeType& SomeVal, bool& Success, ...);

IOW, the caller gives us a pointer to itself, and a place where we can
store a result. We, in turn, either return true if we satisfied the
call synchronously (possibly with a result of failure that we stored
somewhere else!), or else false to indicate we will produce the result
asynchronously. In the latter case, it's expected the calling job will
block itself, and we will wake it after we have stored the desired
result.

Obviously, this call takes place explicitly, manually, cooperatively.
The caller must supply addresses that are persistent, and must be
coded as a state machine that knows "where" to resume processing when
it's vRun() function is next run, indicating a result has been
produced.


 */

void Database::vRun()
    {
    if(runParent)
        this->myRun();
    else
        Database::myRun();
    }

enum {
    INIT    = 0,
    };

void Database::myRun()
    {
    fprintf(stderr, "Database::myRun()\n");
    switch(state)
        {
        case    INIT:   // sanity check: should not get called in this state
            assert(false);
            break;
        }
    }

bool Database::GetSession(string sessionID, SharedRes& result)
    {
    bool completed = false; // assume we'll fail to complete the request

    

    return completed;
    }
