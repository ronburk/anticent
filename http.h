#ifndef HTTP_H_
#define HTTP_H_

#include "poll.h"
#include "job.h"
#include <string>
using std::string;

class HttpConn;

class HttpReader : public Job
    {
    friend class HttpConn;
    HttpConn*   connection;
    HttpReader(HttpConn*);
    string      request;
    bool        Parse();
public:
    virtual void Run();
    };

class HttpConn : public Eventable /* , public Job */
    {
    friend class HttpReader;
    friend class HttpWriter;
    HttpReader*     reader;
    HttpConn(fd_t socket);
protected:
    ssize_t         Read(void* buffer, size_t count);
    ssize_t         Write(void* buffer, size_t count);
public:
    static void     New(fd_t socket);
    virtual void    Event(int event);
    };

#endif /* HTTP_H_ */
