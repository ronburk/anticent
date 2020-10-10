#include "http.h"
#include "ipaddr.h"
#include <cstdio>
#include <unistd.h> // read, write
#include <errno.h>
#include <cassert>
#include <sys/socket.h>

static HttpListener* This;


void Init::NewHttpListener(string nicname, int port, bool IP6)
    {
    assert(This == nullptr); // initial design allows for exactly one HTTP listener.
    This = new HttpListener(this);
    This->Listen(nicname, port, IP6);
    This->Schedule();
    }

void HttpListener::Event(int event)
    {
    fprintf(stderr, "HttpListener gets event.\n");
    fd_t connFd = IPaddr::Accept(fd);

    fprintf(stderr, "HttpListener accepts new connection.\n");
    NewHttpConn(this, connFd);
    }


/* Shutdown() - trivial version
 *
 * Take ourselves out of the epoll interest list, and close our listening socket.
 * Not sure it needs to be any fancier than this. I suppose we could try to empty
 * out connections already in the accept queue if, for example, we knew we were
 * being fronted by a load balancer that has shut off the spigot of new connections.
 */
void HttpListener::Shutdown()
    {
    fd_t socket = This->Del();
    fprintf(stderr, "HttpListener::Shutdown() closing socket\n");
    close(socket);
    }



/* HTTP lifetimes: a bit complex.
 * ==============================
 *
 * HttpConn is not a job, but it manages two jobs: HttpReader and HttpWriter.
 * HttpConn lives as long as the remote client can still talk to it.
 *
 * Note that HTTP requests pipelined on a single connection must be responded to
 * in order. We satisfy this requirement by not allowing HttpReader to create a
 * new HttpRequest until the current one has finished writing its response.
 * There is nothing wrong with letting HttpReader read ahead, of course, it just
 * must not begin acting on any request until the previous one is completed.
 *
 * 
 */

/* HttpConn:

HttpConn is an Eventable that never Run's. It is born with a priority
of BLOCKED and stays that way. It owns an HttpReader and an HttpWriter.
It does its work when its Event() callback is called, at which point it

 */


/* NewConn() -create a new HttpConn object.
 *
 * This is conceptionally part of the interface of HttpConn, which is why
 * we put it here instead of with HttpListener.
 */
void HttpListener::NewHttpConn(Job* parent, fd_t socket)
    {
    fprintf(stderr, "HttpListener::NewHttpConn(parent, [%d]\n", socket);
    auto This =  new HttpConn(parent);
    This->Eventable::Add(socket, EPOLLIN|EPOLLOUT);
    This->reader = new HttpReader(This);
    This->writer = new HttpWriter(This);
    }


HttpConn::HttpConn(Job* parent)
    : Eventable(parent)
    {
    Constructed();
    }


void HttpConn::NewHttpRequest(string requestText)
    {
    new HttpRequest(this, writer, requestText);
    }

/* HttpConn::Event() - handle I/O event on this connection.

 */
void HttpConn::Event(int event)
    {
    fprintf(stderr, "HttpConn::Event(%08X)\n", event);
    // if we got a reads-are-unblocked event
    if((event & EPOLLIN) != 0)
        if(reader)
            {
            fprintf(stderr, "HttpConn::Event() schedules reader\n");
            reader->Schedule(JobPriority::LOW);
            }
    // if we got a writes-are-unblocked event
    if((event & EPOLLOUT) != 0)
        if(writer && !writer->buffer.empty() && writer->IsBlocked())
            {
            fprintf(stderr, "HttpConn::Event() schedules writer\n");
            writer->Ready();
            }
    }

ssize_t HttpConn::Read(char* buffer, ssize_t count)
    {
    ssize_t result;

    result  = read(Eventable::fd, buffer, count);
    fprintf(stderr, "HttpConn::Read([%d], %zu) = %ld\n", Eventable::fd, count, result);
    // if client has closed their write side
    if(result == 0)
        {
        fprintf(stderr, "HttpConn::Read(): client closed our input\n");
        shutdown(Eventable::fd, SHUT_RD);
        }
    return result;
    }

ssize_t HttpConn::Write(const char* buffer, ssize_t count)
    {
    ssize_t result = 0;

    fprintf(stderr, "HttpConn::Write(%zu) bytes on [%d]\n", count, Eventable::fd);
    result  = write(Eventable::fd, buffer, count);
    fprintf(stderr, "HttpConn::Write(%zu) bytes returns %zd\n", count, result);
    if(result < count)
        writer->Block();
    fprintf(stderr, "HttpConn::Write(%zu) bytes returns %zd\n", count, result);
    return result;
    }

/*=============================
 * HttpReader
 */


HttpReader::HttpReader(Job* parent) : Job(parent)
    {
    Constructed();
    }

/* ??? for now, only handle GET with no body
 */
bool HttpReader::Parse()
    {
    bool gotone = false;
    auto end = request.find("\r\n\r\n");
    if(end != string::npos)
        {
        string newReq = request.substr(0, end);
        fprintf(stderr, "Found request!:\n%s\n", newReq.c_str());
        request.erase(0, end+4);
        fprintf(stderr, "left over Found request!:\n'%s'\n", request.c_str());
        gotone = true;
        static_cast<HttpConn*>(parent)->NewHttpRequest(newReq);
        }
    return gotone;
    }
/* Run()

General philosophy: highest priority is finishing a request, lowest
priority is starting a new request. HttpReader is on the "starting a
new request" end of things, so we want it to be low priority.

Also, in the case of pipelining, HttpReader must not create a new
Request until the previous Request is completed. Ooops, that's not true
since we may be acting as a proxy. So, instead, we must identify the session
this request belongs to, and sequentialize requests that belong to the
same session. Probably, we will have to end up being aware whether our
connection is from a client or a proxy server to do this correctly.

??? must make sure it's not possible to get proxy config that allows
requests from same client on same TCP connection to arrive here out of order!
Out of order requests when the client is using more than one TCP connection
is a known problem that belongs to the client programmer.

state 0: not scheduled (Job::next is null)
    EPOLLIN event: HttpConn schedules us low priority -> state 1
state 1: (continue) reading next request
    EAGAIN -> state 0
    request read complete, no request in-flight, spin off request -> state 1
    request read complete, request in-flight -> state 2
state 2: waiting to read next request
    in-flight request completes, schedules us low priority -> state 1

 * remember: when Run() is called, this Job is removed being scheduled.
 */
void HttpReader::vRun()
    {
    assert(parent != nullptr);
    const   int BUFSIZE = 255;
    char    buffer[BUFSIZE+1];
    auto    connection = static_cast<HttpConn*>(parent);

    ssize_t nbytes = 0;
    bool    done   = false;
    while(!done)
        {
        nbytes = connection->Read(buffer, BUFSIZE);
        fprintf(stderr, "read %ld bytes of http request.\n", nbytes);
        perror("reader");
        if(nbytes >= 0)
            {
            buffer[nbytes] = '\0';
            request += buffer;
            if(Parse())
                done    = true;
            }
        fprintf(stderr, "if %zd < %d\n", nbytes, BUFSIZE);
        if(nbytes < BUFSIZE)
            {
            fprintf(stderr, "reader blocks itself\n");
            Block();
            done    = true;
            }
        }
    if(nbytes == -1)
        {
        fprintf(stderr, "==============errno=%d\n", errno);
        switch(errno)
            {
            case    EINTR   :
                // The call was interrupted by a signal before any data was read.
                // In that case, I claim we should just try again if the scheduler
                // is game.
                break;
#if EAGAIN != EWOULDBLOCK
            case    EAGAIN :
#endif
            case    EWOULDBLOCK:
                // in this case, Block() was already called
                break;
            default:
                perror("HttpReader::Run() call to read().");
                assert(false);
            }
        }
    }

HttpRequest::HttpRequest(Job* parent, HttpWriter* writer, string requestText)
    : Job(parent, JobPriority::LOW), writer(writer)
    {
    Constructed();
    }

void HttpRequest::vRun()
    {
    const char* response = ""
"HTTP/1.1 200 OK\r\n"
"Date: Mon, 27 Jul 2009 12:28:53 GMT\r\n"
"Server: Apache/2.2.14 (Win32)\r\n"
"Last-Modified: Wed, 22 Jul 2009 19:15:56 GMT\r\n"
"Content-Length: 0\r\n"
"Content-Type: text/html\r\n"
"Connection: Closed\r\n\r\n";

    fprintf(stderr, "HttpRequest::vRun()\n");
    static_cast<HttpWriter*>(writer)->Write(response, strlen(response));
    Block();
    }
JobPriority HttpRequest::vBasePriority()
    {
    return JobPriority::LOW;
    }

/* HttpWriter:

Can be blocked for two different reasons. First, it has simply written
everything it can to the associated HttpConn. In that state, it should
unblock when somebody writes more data to it. Second, it was unable to
write everything it can because HttpConn blocked it. In that state,
it should unblock when there is a EPOLLOUT I/O event.

 */

HttpWriter::HttpWriter(Job* parent)
    : Job(parent, JobPriority::BLOCKED)
    {
    Constructed();
    }

void    HttpWriter::vRun()
    {
    fprintf(stderr, "HttpWriter::vRun() returns\n");

    // if there are bytes waiting to go out
    if(buffer.size() > 0)
        {
    fprintf(stderr, "HttpWriter::vRun() returns\n");
        auto nBytes = static_cast<HttpConn*>(parent)->Write(&buffer[0], buffer.size());
    fprintf(stderr, "HttpWriter::vRun() returns\n");
        if(nBytes > 0)
            buffer.erase(0, nBytes);
    fprintf(stderr, "HttpWriter::vRun() returns\n");
        }
    fprintf(stderr, "HttpWriter::vRun() returns\n");
    if(buffer.size() == 0)
        Block();
    fprintf(stderr, "HttpWriter::vRun() returns\n");
    }

JobPriority HttpWriter::vBasePriority()
    {
    return JobPriority::HIGH;
    }

void HttpWriter::Write(const char* otherBuffer, ssize_t count)
    {
    fprintf(stderr, "HttpWriter::Write()\n");
    // if no prior work queued up
    if(buffer.size() == 0)
        Ready();    // worst case (unlikely), we just get blocked

    buffer.append(otherBuffer, count);
    }
