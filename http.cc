#include "http.h"
#include <cstdio>
#include <unistd.h> // read, write
#include <errno.h>

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

void HttpConn::New(fd_t socket)
    {
    new HttpConn(socket);
    }

HttpConn::HttpConn(fd_t socket)
    {
    Eventable::Add(socket, EPOLLIN|EPOLLOUT);
    reader = new HttpReader(this);
    }

void HttpConn::Event(int event)
    {
    fprintf(stderr, "HttpConn::Event(%08X)\n", event);
    if((event & EPOLLIN) != 0)
        {
        if(reader && reader->IsBlocked())
            {
            fprintf(stderr, "HttpConn::Event() schedules reader\n");
            reader->Schedule();
            }
        }
    }

ssize_t HttpConn::Read(void* buffer, size_t count)
    {
    ssize_t result;

    result  = read(Eventable::fd, buffer, count);
    fprintf(stderr, "HttpConn::Read(%zu) = %ld\n", count, result);
    return result;
    }
ssize_t HttpConn::Write(void* buffer, size_t count)
    {
    ssize_t result;

    result  = write(Eventable::fd, buffer, count);
    return result;
    }

/*=============================
 * HttpReader
 */


HttpReader::HttpReader(HttpConn* parent) : connection(parent)
    {
    SetPriority(Job::PRI_LOW); // creating new requests is low priority
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
        gotone = true;
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
void HttpReader::Run()
    {
    char    buffer[256];
    ssize_t nbytes;

    for(nbytes=0; nbytes >= 0;)
        {
        nbytes = connection->Read(buffer, 255);
        fprintf(stderr, "read %ld bytes of http request.\n", nbytes);
        perror("reader");
        if(nbytes >= 0)
            {
            buffer[nbytes] = '\0';
            request += buffer;
            if(Parse())
                break;
            }
        }
    if(nbytes == -1)
        {
        switch(errno)
            {
            case    EINTR   :
                // The call was interrupted by a signal before any data was read.
                // In that case, I claim we should just try again if the scheduler
                // is game.
                Schedule();
                break;
#if EAGAIN != EWOULDBLOCK
            case    EAGAIN :
#endif
            case    EWOULDBLOCK:
                // we are blocked for reading. remain unscheduled, epoll will wake us
                break;
            default:
                perror("HttpReader::Run() call to read.");
                assert(false);
            }
        }
    }
