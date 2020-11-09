#ifndef DB_H_
#define DB_H_

#include <string>
using std::string;

#include "resource.h"
#include "job.h"

/* db.h - abstract interface to database.
 *
 * uid, gid, res, resLayer
 */

class Database;
class Database : public Job
    {
    short           state       = 0;
    bool            runParent   = true;
    SharedRes       session     = nullptr;
protected:
    virtual void    vRun() final;
    virtual void    myRun() = 0; // must be overridden
public:
    Database(Job* parent)
        : Job(parent) {}
    bool            GetSession(string sessionID, SharedRes& result);
    };

#if 0
class Database
    {
public:
    Database();
   ~Database();
//    char* Read(
    static cid_t                SyncCache();
    static unique_ptr<Session>  GetSession(cid_t cid, string sessionID);
    static SharedRes            Get(Session* Session, string Path);
    };
#endif
#endif /* DB_H_ */
