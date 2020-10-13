#ifndef HTTP_H_
#define HTTP_H_

#include "init.h"
#include "listener.h"
#include <string>
using std::string;

class HttpListener : public Listener
    {
    friend void Init::NewHttpListener(string nicname, int port, bool IP6);

    HttpListener(Job* parent):Listener(parent){}
protected:
    virtual const char*  vClassName() { return "HttpListener"; }

public:
    static void     Shutdown();
    static void     New(string nicname, int port, bool IP6=false);
    void            NewHttpConn(Job* parent, fd_t sock);
    fd_t            Accept();
    virtual void    Event(int event);
    };

class HttpConn;

class HttpReader : public Job
    {
    HttpReader(Job* parent);
    string      request;
    bool        Parse();
private:
    friend void HttpListener::NewHttpConn(Job*, fd_t);
    friend class HttpConn;
protected:
    virtual const char*  vClassName() { return "HttpReader"; }
    virtual void vRun();
public:
    };

class HttpWriter : public Job
    {
    HttpWriter(Job* parent);
    string          response;
    string          buffer;
private:
    friend void HttpListener::NewHttpConn(Job*, fd_t);
    friend class HttpConn;
protected:
    virtual void        vRun();
    virtual short vBasePriority();
    virtual const char*  vClassName() { return "HttpWriter"; }
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

class HttpConn : public Eventable
    {
    HttpConn(Job* parent);
    HttpReader*     reader;
    HttpWriter*     writer;
    bool            
private:
    friend void     HttpListener::NewHttpConn(Job*, fd_t);
    friend class    HttpReader;
    friend class    HttpWriter;
protected:
    virtual const char*  vClassName() { return "HttpConn"; }
    virtual void    vDeathRequest(Job* dying);
    ssize_t         Read(char* buffer, ssize_t count);
    ssize_t         Write(const char* buffer, ssize_t count);
public:
    void            NewHttpRequest(string requestText);
    virtual void    Event(int event);
    };


class HttpRequest : public Job
    {
    friend void HttpConn::NewHttpRequest(string requestText);

    HttpRequest(Job* parent, HttpWriter* writer, string requestText);
    HttpWriter* writer;
protected:
    virtual const char*  vClassName() { return "HttpRequest"; }
    virtual short vBasePriority();
    virtual void        vRun();
public:
    };

#endif /* HTTP_H_ */
