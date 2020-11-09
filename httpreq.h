#ifndef HTTPREQ_H
#define HTTPREQ_H

#include "http.h"
#include "job.h"
using std::shared_ptr;

#include "db.h"

class   Session
    {
public:
    int     userID  = 1;
    int     groupID = 1;
    };


class HttpRequest : public Database
    {
    friend void HttpConn::NewHttpRequest(string requestText);
    friend void HttpConn::vDeathRequest(Job* dying);

    HttpRequest(Job* parent, HttpWriter* writer, string requestText);
   ~HttpRequest();
    string              GetSessionID();
    shared_ptr<Session> GetSession(string sessionID);
    string              GetCookie(const string& name);
    string              GetHeader(const string& name);
    HttpWriter*         writer;
    string              requestText;
    int                 state;
protected:
    virtual const char* vClassName() { return "HttpRequest"; }
    virtual short       vBasePriority();
    virtual void        myRun();
public:
    };


#endif /* HTTPREQ_H */
