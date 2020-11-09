#include "httpreq.h"
#include <cstring>
#include <string.h>

enum
    {
    INIT        = 0,
    };


HttpRequest::HttpRequest(Job* parent, HttpWriter* writer, string requestText)
    : Database(parent),
      writer(writer),
      requestText(requestText),
      state(0)
    {
    // born unscheduled
    Constructed();
    }
HttpRequest::~HttpRequest()
    {
    // should have gotten scheduled before dying!
    Unschedule();
    }

void HttpRequest::myRun()
    {
    switch(state)
        {
        case    INIT:
            
#if 0
            if(!SyncCache())
                return;
            else
                state = GOTCID;
            break;
#endif
            auto sessionID  = GetSessionID();
            auto session    = GetSession(sessionID);
            break;
        }
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

// ??? ha-ha fragile
string HttpRequest::GetCookie(const string& name)
    {
    string result;
    string cookieString = GetHeader("Cookie");

    if(!cookieString.empty()) // if we found a Cookie header
        {
        string find = name + "=";
        const char* nameRover = cookieString.c_str();
        while(strchr(" \t", *nameRover)) // skip leading space
            ++nameRover;
        while(*nameRover) // while not at end of string
            {
            const char* pair = strcasestr(whole.c_str(), find.c_str());
            if(pair) // if we found another name=value pair
                {
                
                }
            }
        }
    return result;
    }


// ??? assumes no body
// ??? so fragile!
// ??? can't distinguish no header from empty header
string HttpRequest::GetHeader(const string &name)
    {
    string result;

    string pattern = "\n" + name + ":";
    const char* header = requestText.c_str();
    header   = strcasestr(header, name.c_str());
    if(header)
        {
        fprintf(stderr, "[%s](%s)='%s'\n",
            __PRETTY_FUNCTION__, name.c_str(), header);
        const char* value = strstr(header, ":");
        assert(value != nullptr);
        while(strchr(" \t", *value))
            ++value;
        if(!strchr("\r\n", *value))
            {
            const char* rover = value;
            while(!strchr("\r\n", *rover))
                ++rover;
            result  = string(value, rover);
            }
        }

    return result;
    }

string HttpRequest::GetSessionID()
    {
    string cookie = GetHeader("Cookie");
    fprintf(stderr, "cookie = '%s'\n", cookie.c_str());
    if(cookie.empty())
        {
        return "";
        }
    else
        assert(false);
#if 0
    // ??? use /dev/urandom and base64 encoding
    static  int seed=0;
    char    result[256];
    sprintf(result, "%d", seed);
    ++seed;
    return result;
#endif
    }

shared_ptr<Session> HttpRequest::GetSession(string sessionID)
    {
    return std::make_shared<Session>();
    }
