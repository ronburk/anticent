#include "http.h"
#include "ipaddr.h"
#include <cstdio>
#include <unistd.h> // read, write
#include <errno.h>
#include <cassert>
#include <sys/socket.h>
#include <fcntl.h> // fcntl
#include "signals.h"

/*
we need to linger

The state is really three parts:
a) connection
b) reader
c) writer

 */
enum
    {
    CONNSTART,      // 0 initial state
    CONNIONOWORK,   // 1 two-way I/O is possible
    CONNIOWORKING,  // 2 working on requests
    CONNWRITEONLY,  // 3 RD is closed, still sending response(s)
    CONNREADONLY,   // 4 FIN sent, reading/dumping input
    CONNCLOSED,     // 5 ready to close socket
    CONNDEATH,      // 6
//7-+V    
    ECONNIO,        // 7
    EREADOPEN,      // 8
    EREADCLOSED,    // 9  got zero bytes when reading
    EWRITEOPEN,     // 10
    ECLOSEWRITE,    // 11 the write side is shut down.
    EWORKTODO,      // 12
    ENOWORK,        // 13
    EFINALREQUEST,  // 14request completes and request queue is empty
    EHUP,           // 15 HUP
    EERR,           // 16 EPOLLERR
    EDEATH,         // 17
    };

//int state       = CONNSTART;
//int runState    = 0;
void HttpConn::NextState(int event)
    {
    bool    error       = false;
    int     nextState   = state;

    switch(state)
        {
        case    CONNSTART : // initial state
            switch(event)
                {
                case    EERR:
                case    EHUP:
                    Close();
                    nextState   = CONNCLOSED;
                    break;
                case    EWORKTODO: // have a request to work on
                    nextState = CONNIOWORKING;
                    break;
                default : error = true;
                }
            break;
        case    CONNIONOWORK :    // two-way traffic, but no requests to do
            switch(event)
                {
                case    EREADCLOSED:
                    Close();
                    nextState = CONNDEATH;
                    break;
                case    ECLOSEWRITE:
                    nextState = CONNREADONLY;
                    CloseWrite();
                    break;
                case    EWORKTODO:
                    nextState = CONNIOWORKING;
                    break;
                case    EHUP:
                    Close();
                    nextState   = CONNCLOSED;
                    break;
                default : error = true;
                };
            break;
        case    CONNIOWORKING: // Working on requests, and 2-way channel open
            switch(event)
                {
                case    ENOWORK:
                    nextState = CONNIONOWORK;
                    break;
                case    EREADCLOSED:
                    nextState = CONNWRITEONLY;
                    break;
                default : error = true;
                }
            break;
        case    CONNWRITEONLY:
            switch(event)
                {
                case    ECLOSEWRITE:
                    nextState   = CONNCLOSED;
                    break;
                default : error = true;
                }
            break;
        case    CONNREADONLY:
            switch(event)
                {
                case    EREADCLOSED:
                    nextState   = CONNCLOSED;
                    break;
                case    EHUP:
                    Close();
                    nextState   = CONNDEATH;
                    break;
                default : error = true;
                }
            break;
        case    CONNCLOSED:
            switch(event)
                {
                case    EFINALREQUEST:
                    break;
                default : error = true;
                }
            break;
        case    CONNDEATH:
            DeathRequest();
            break;
        default : error = true;
        }
    fprintf(stderr, "======================== [%d]-%d->[%d]\n", state, event, nextState);
    assert(error == false);
    state   = nextState;
    if(state == CONNDEATH)
        DeathRequest();
    }

/* HttpConn::Close() - force a socket close.
 *
 * We can no longer send or receive data with the client. However, some
 * of our child HttpRequest Jobs might still be processed.
 */
void HttpConn::Close()
    {
    fprintf(stderr, "HttpConn::Close()!  Yay!!\n");
    // can safely delete reader, nobody else can call it
    if(reader != nullptr)
        {
        delete reader;
        reader  = nullptr;
        }
#if 0
    assert(writer == nullptr);
    assert(request == nullptr);
    assert(requests.Count() == 0);
#endif
    close(Eventable::fd); // close removes from epoll interest list
    }


static HttpListener* This;


void Init::NewHttpListener(string nicname, int port, bool IP6)
    {
    assert(This == nullptr); // initial design allows for exactly one HTTP listener.
    This = new HttpListener(this);
    This->Listen(nicname, port, IP6);
    }

HttpListener::HttpListener(Job* parent)
    :   Listener(parent)
    {
    Signals::Subscribe(this, 0);
    }
HttpListener::~HttpListener()
    {
    }
void    HttpListener::vSignal(int signum)
    {
    fprintf(stderr, "[%s] signum=%d, connCount=%d\n",
        __PRETTY_FUNCTION__, signum, connCount);
    shuttingDown = true;
    if(connCount <= 0)
        DeathRequest();
    }

void HttpListener::vDeathRequest(Job* dying)
    {
    fprintf(stderr, "[%s] gets death request from %s. %d connections.\n",
        __PRETTY_FUNCTION__, dying->ClassName(), connCount);
    delete dying;
    if(--connCount <= 0 && shuttingDown)
        DeathRequest();
    }

void HttpListener::Event(int event)
    {
    fprintf(stderr, "HttpListener gets event.\n");
    fd_t connFd = IPaddr::Accept(fd);

    fprintf(stderr, "[%s] accepts new connection.\n",
        __PRETTY_FUNCTION__);
    NewHttpConn(this, connFd);
    ++connCount;
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
    This->Close();
    fprintf(stderr, "HttpListener::Shutdown() closing socket\n");
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
    This->writer = new HttpWriter(This);
    This->reader = new HttpReader(This);
    This->Eventable::Add(socket, EPOLLIN|EPOLLOUT|EPOLLRDHUP|EPOLLHUP|EPOLLERR);
    }


HttpConn::HttpConn(Job* parent)
    : Job(parent), state(CONNSTART), reader(nullptr),
      writer(nullptr), request(nullptr)
    {
    Constructed();
    }

HttpConn::~HttpConn()
    {
    fprintf(stderr, "delete reader\n");
    delete reader;
    fprintf(stderr, "delete writer\n");
    delete writer;
    assert(request == nullptr);
    assert(requests.Count() == 0);
//  assert(state == CONNDEATH);
    }


/* HttpConn::NewHttpRequest() - create (and own) a new incoming HTTP request.
 */
void HttpConn::NewHttpRequest(string requestText)
    {
    auto request = new HttpRequest(this, writer, requestText);

    if(this->request) // if already working on request
        {
        fprintf(stderr, "Queue this request\n");
        requests.Push(request); // and add to our private job list
        }
    else
        {
        request->Schedule(Job::LOW, Job::LOW);
        this->request = request;
        }
    NextState(EWORKTODO);
    }

/* HttpConn::Event() - handle I/O event on this connection.

Note that edge-triggered epoll events deliver all the flags
representing the current state of the socket. If you want to know what
"event" really happened, you must keep a copy of the previous state
and compare it to see what actually changed.

Note also, that multiple events could have accumulated since the last
time we checked. Sometimes we must deliver them all, other times not.
For example, if an EPOLLERR occurred, it doesn't matter what else
changed since the last check -- we're shutting that connection down.

 */
void HttpConn::Event(int event)
    {
    fprintf(stderr, "HttpConn::Event(%08X)\n", event);
    // if error or hup, nothing else matters
    if((event&EPOLLERR) != 0)
        NextState(EERR);
    else if((event&EPOLLHUP) != 0)
        NextState(EHUP);
//    else if(state == CONNSTART && (event&(EPOLLIN|EPOLLOUT)) == (EPOLLIN|EPOLLOUT))
//        NextState(ECONNIO);
    else
        {
        // if writes are (possibly "still") unblocked
        fprintf(stderr, "check epollout\n");
        if((event&EPOLLOUT) != 0)
            {
            fprintf(stderr, "check epollout writer (%p)\n", writer);
            if(!writer->buffer.empty() && writer->IsBlocked())
                {
                fprintf(stderr, "HttpConn::Event() schedules writer\n");
                writer->Ready();
                }
            }
        // if reads are (still?) unblocked
        if((event&EPOLLIN) != 0)
            {
            if(reader->IsBlocked())
                {
                fprintf(stderr, "HttpConn::Event() schedules reader\n");
                reader->Ready();
                }
            }
        }
    }

void    HttpConn::CloseWrite()
    {
    int status = shutdown(Eventable::fd, SHUT_WR);
    if(status != 0)
        {
        perror("shutdown(SHUT_WR)");
        assert(false);
        }
    }

void    HttpConn::vDeathRequest(Job* dying)
    {
    if(dying == reader) // if the reader is giving up
        {               // then it's OK to half-close
        fprintf(stderr, "HttpConn::Read(): client closed our input %d\n", errno);
        int status = shutdown(Eventable::fd, SHUT_RD);
        if(status != 0)
            {
            perror("shutdown(SHUT_RD)");
            assert(false);
            }
        NextState(EREADCLOSED);
        delete reader;
        reader = nullptr;
        }
    else if(dying == request)
        {
        delete request;
        request = static_cast<HttpRequest*>(requests.Pop());
        if(request)
            request->Schedule();
        else
            {
            NextState(ENOWORK);
            // if zero current requests and zero unwritten response bytes
            if(reader == nullptr && writer && writer->buffer.empty())
                {
                fprintf(stderr, "ECLOSEWRITE because no possible output now\n");
                NextState(ECLOSEWRITE);
                }
            }
        }
    else if(dying == writer)
        {
        delete writer;
        writer  = nullptr;
        // writer would not die if any requests left
        fprintf(stderr, "ECLOSEWRITE because writer dying\n");
        NextState(ECLOSEWRITE);
        }
    }

/* HttpConn::Read() - socket-level read function.

Keep in mind well-known problem: if server closes its read side while
client is still writing, TCP stack will send a RST (reset) to the
client, which may then discard significant received data that hasn't
made it to user space.  Upshot is that we generally want reader alive
until the client closes, even if it's just discarding data it's not
going to use.

How do we detect the client has closed the write side of their
connection?  We get an EPOLLIN event on the socket and the very next
read() returns 0. The logic goes like this:
a) HttpReader must exhaust input to get a new EPOLLIN event.
b) HttpReader goes BLOCKED if it exhausts input
c) HttpReader only unblocks for EPOLLIN event
d) So if first read of HttpReader->vRun gets 0 bytes, writer is done.

 */
ssize_t HttpConn::Read(char* buffer, ssize_t count)
    {
    ssize_t result;

    result  = read(Eventable::fd, buffer, count);
    fprintf(stderr, "HttpConn::Read([%d], %zu) = %ld\n", Eventable::fd, count, result);
    // if client has closed their write side
    return result;
    }

ssize_t HttpConn::Write(const char* buffer, ssize_t count)
    {
    ssize_t result = 0;
    fprintf(stderr, "HttpConn::Write(buffer, %zd)\n", count);

// ??? must track requests and last request
    if(count == 0)  // if that's all the data for this request
        {
        fprintf(stderr, "%s: End of response.\n", __func__);
        if(reader == nullptr && true) // ??? if no more requests to process
            {
            shutdown(Eventable::fd, SHUT_WR);
//            close(Eventable::fd);  // remove from epoll, etc.
            }
        }
    else
        {
        result  = write(Eventable::fd, buffer, count);
        fprintf(stderr, "HttpConn::Write(%zu) bytes on [%d] returns %zd\n",
            count, Eventable::fd, result);
        if(result <= count)
            writer->Block();
        }
    return result;
    }

/*=============================
 * HttpReader
 */


HttpReader::HttpReader(Job* parent) : Job(parent)
    {
    Schedule(Job::BLOCKED, Job::LOW);
    Constructed();
    }
HttpReader::~HttpReader()
    {
    fprintf(stderr, "~HttpReader()\n");
    Unschedule();
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
/*
  events:
      Try to look at the world from reader's perspective.
          EPOLLIN - wake up and read
          
      Reall, we need EPOLLIN (you have something to read). And 
  EPOLLRDHUP
  EPOLLIN
  EPOLLHUP
 */

/* HttpReader ======================

Born:
    When HttpListener::NewHttpConn() creates a new HttpConn.
Owned by:
    Parent HttpConn.
Dies:
    a) when it detects read side of socket is closed.
    b) or when parent HttpConn dies.

 */

void HttpReader::vRun()
    {
    assert(parent != nullptr);
    const   int BUFSIZE = 255;
    char    buffer[BUFSIZE+1];

    ssize_t nbytes = 0;
    bool    done   = false;
    for(int iRead = 0; !done; ++iRead)
        {
        nbytes = Parent()->Read(buffer, BUFSIZE);
        fprintf(stderr, "read %ld bytes of http request.\n", nbytes);
        if(iRead == 0 && nbytes == 0) // if client closed their write-side
            {
            fprintf(stderr, "HttpReader begs for death.\n");
//            DeathRequest();
            Parent()->NextState(EREADCLOSED);
            return;
            }
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
        perror("reader");
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
    : Job(parent), writer(writer)
    {
    // born unscheduled
    Constructed();
    }
HttpRequest::~HttpRequest()
    {
    // should have gotten scheduled before dying!
    Unschedule();
    }

void HttpRequest::vRun()
    {
    const char* response = ""
"HTTP/1.1 404 OK\r\n"
"Date: Mon, 27 Jul 2009 12:28:53 GMT\r\n"
"Server: Apache/2.2.14 (Win32)\r\n"
"Last-Modified: Wed, 22 Jul 2009 19:15:56 GMT\r\n"
"Content-Length: 0\r\n"
"Content-Type: text/html\r\n"
"Connection: close\r\n\r\n";

    fprintf(stderr, "HttpRequest::vRun()\n");
// ??? check for blocking
    writer->Write(response, strlen(response));
//    writer->Write(response, 0); // end of response
    // only when done ???
    DeathRequest();
    }

short HttpRequest::vBasePriority()
    {
    return Job::LOW;
    }

/* HttpWriter:

Can be blocked for two different reasons. First, it has simply written
everything it can to the associated HttpConn. In that state, it should
unblock when somebody writes more data to it. Second, it was unable to
write everything it can because HttpConn blocked it. In that state,
it should unblock when there is a EPOLLOUT I/O event.

 */

HttpWriter::HttpWriter(Job* parent)
    : Job(parent)
    {
    Schedule(Job::BLOCKED, Job::HIGH);
    Constructed();
    }
HttpWriter::~HttpWriter()
    {
    Unschedule();
    }


void    HttpWriter::vRun()
    {
    fprintf(stderr, "HttpWrite::vRun() buffer.size = %zd\n", buffer.size());
    assert(buffer.size() > 0);
    auto nBytes = Parent()->Write(&buffer[0], buffer.size());
    if(nBytes < 0)     // if we got some error
        {
        perror("HttpWriter::vRun()");
        assert(false);
        }
    else
        {
        buffer.erase(0, nBytes);
        if(nBytes <= long(buffer.size()))
            Block();
//        else if(Parent()->request == nullptr)
//            DeathRequest();
        }

    fprintf(stderr, "HttpWriter::vRun() returns\n");
    }

short HttpWriter::vBasePriority()
    {
    fprintf(stderr, "HttpWriter::vBasePriority() returns %d\n", Job::HIGH);
    return Job::HIGH;
    }

void HttpWriter::Write(const char* otherBuffer, ssize_t count)
    {
    fprintf(stderr, "HttpWriter::Write(otherBuffer, %zd)\n", count);
    // if no prior work queued up
    if(buffer.size() == 0)
        Ready();    // worst case (unlikely), we just get blocked

    buffer.append(otherBuffer, count);
    fprintf(stderr, "  returns\n");
    }
