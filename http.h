#ifndef HTTP_H_
#define HTTP_H_

#include "init.h"
#include "listener.h"
#include <cassert>
#include <string>
using std::string;

inline
int     EventBit(int bit, int event, int prevEvent)
    {
    return ((prevEvent&bit) == 0)
        && (event&bit) != 0;
    }

class HttpListener : public Listener
    {
    friend void Init::NewHttpListener(string nicname, int port, bool IP6);

    HttpListener(Job* parent);
   ~HttpListener();
    bool            shuttingDown = false;
    int             connCount = 0;
protected:
    virtual const char* vClassName() { return "HttpListener"; }
    virtual void        vRun() { assert(false); }
    virtual void        vDeathRequest(Job* dying);
    virtual void        vSignal(int signum);

public:
    static void     Shutdown();
    static void     New(string nicname, int port, bool IP6=false);
    void            NewHttpConn(Job* parent, fd_t sock);
    fd_t            Accept();
    virtual void    Event(int event);
    };

class HttpRequest;
class HttpReader;
class HttpWriter;
class HttpConn : public Eventable, public Job
    {
    int             state;
    HttpReader*     reader;
    HttpWriter*     writer;
    HttpRequest*    request;
    DList           requests;
    HttpConn(Job* parent);
   ~HttpConn();
    HttpListener*   Parent() { return static_cast<HttpListener*>(parent); }
    void            CloseWrite();
    void            Close();
private:
    friend void     HttpListener::NewHttpConn(Job*, fd_t);
    friend class    HttpReader;
    friend class    HttpWriter;
protected:
    virtual const char*  vClassName() { return "HttpConn"; }
    ssize_t         Read(char* buffer, ssize_t count);
    ssize_t         Write(const char* buffer, ssize_t count);
    void            NextState(int event);
public:
    virtual void    vDeathRequest(Job* dying);
    void            NewHttpRequest(string requestText);
    virtual void    Event(int event);
    void            Schedule(short,short) = delete;
    void            Unschedule() = delete;
    };

class HttpReader : public Job
    {
    HttpReader(Job* parent);
   ~HttpReader();
    HttpConn*   Parent() { return static_cast<HttpConn*>(parent); }
    string      request;
    bool        Parse();
private:
    friend void HttpListener::NewHttpConn(Job*, fd_t);
    friend class HttpConn;
protected:
    virtual const char* vClassName() { return "HttpReader"; }
    virtual void        vRun();
public:
    };

class HttpWriter : public Job
    {
    HttpWriter(Job* parent);
   ~HttpWriter();
    HttpConn*   Parent() { return static_cast<HttpConn*>(parent); }
    string          response;
    string          buffer;
private:
    friend void HttpListener::NewHttpConn(Job*, fd_t);
    friend class HttpConn;
protected:
    virtual void        vRun();
    virtual short       vBasePriority();
    virtual const char* vClassName() { return "HttpWriter"; }
public:
    void                Write(const char* buffer, ssize_t count);
    };

/*
 events

EPOLLIN
EPOLLOUT
EPOLLRDHUP   (same as read of 0)
REQSTARTED
REQFINISHED
REQFINAL


 */

inline int BitChanged(int bit, int was, int is)
    {
    return bit & (was & is);
    }



#if 0
class HttpRequest : public Job
    {
    friend void HttpConn::NewHttpRequest(string requestText);
    friend void HttpConn::vDeathRequest(Job* dying);

    HttpRequest(Job* parent, HttpWriter* writer, string requestText);
   ~HttpRequest();
    HttpWriter* writer;
protected:
    virtual const char*  vClassName() { return "HttpRequest"; }
    virtual short vBasePriority();
    virtual void        vRun();
public:
    };
#endif
#endif /* HTTP_H_ */
