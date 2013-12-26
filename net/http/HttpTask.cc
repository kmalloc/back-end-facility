#include "HttpTask.h"

#include "net/http/HttpRequest.h"
#include "net/http/HttpRequest.h"

#include "sys/Log.h"

#include <map>
#include <string>
#include <time.h>

static void DefaultHttpRequestHandler(const HttpRequest& req, HttpResponse& response)
{
    response.SetShouldResponse(true);
    response.SetStatusCode(HttpResponse::HSC_200);
    response.SetStatusMessage("OK");

    using namespace std;

    const map<string, string>& headers = req.GetHeader();
    map<string, string>::const_iterator it = headers.begin();

    string body ;
    body.reserve(128);

    body = "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 1.0 Frameset//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-Frameset.dtd\">\
<html>\
<head>\
    <meta http-equiv=\"Content-Type\" content=\"text/html; charset=gb2312\" />\
    <title>miliao http server</title>\
</head>\
<body>\
 <p>";

    body += "Page auto-generated from http request header:\r\n";
    body += "</p>";
    body += "<p>";
    while (it != headers.end())
    {
        body += it->first;
        body += ":";
        body += it->second;
        body += "</p>";

        ++it;
    }

    body += "</p>";
    body += "Body from request: </p> " + req.GetHttpBody();

    body += "</p>\
             </body>\
             </html>";

    response.SetBody(body.c_str());

    char bodylen[32] = {0};
    snprintf(bodylen, 32, "%d", body.size());

    response.AddHeader("Connection", "close");
    response.AddHeader("Host", "miliao server");
    response.AddHeader("Content-Type", "text/html;charset=utf-8");
    response.AddHeader("Content-Length", bodylen);

    time_t now;
    struct tm gm;
    char tmp[64];

    now = time(NULL);
    gm = *gmtime(&now);

    strftime(tmp, sizeof(tmp), "%a, %d %b %Y %H:%S %Z", &gm);
    response.AddHeader("Date", tmp);

    response.SetCloseConn(false);
}

static const int msg_queue_sz = 64;

HttpTask::HttpTask(SocketServer* server, LockFreeBuffer& alloc, LockFreeBuffer& msgPool)
    : ITask(false)
    , tcpServer_(server)
    , context_(*server, alloc, &DefaultHttpRequestHandler)
    , msgPool_(msgPool)
    , sockMsgQueue_(msg_queue_sz)
{
    pthread_mutex_init(&lock_, NULL);
}

HttpTask::~HttpTask()
{
    ReleaseTask();
    pthread_mutex_destroy(&lock_);
}

void HttpTask::ReleaseTask()
{
    context_.ReleaseContext();
    SetAffinity(-1);
}

bool HttpTask::PostSockMsg(SocketEvent* msg)
{
    return sockMsgQueue_.Push(msg);
}

void HttpTask::ResetTask(int connid, int affinity)
{
    context_.ResetContext(connid);
    SetAffinity(affinity);
}

void HttpTask::ProcessHttpData(const char* data, size_t sz)
{
    context_.ProcessHttpRequest(data, sz);
}

void HttpTask::ProcessSocketMessage(SocketEvent* msg)
{
    switch (msg->code)
    {
        case SC_ACCEPT:
            {
                int id = msg->msg.ud;
                int affinity = ITask::GetThreadId();

                ResetTask(id, affinity);

                slog(LOG_INFO, "httptask accept(%d)", id);
            }
            break;
        case SC_CLOSE:
            {
                int id = msg->msg.fd;
                slog(LOG_INFO, "httptask closed(%d)", msg->msg.fd);
                ReleaseTask();
            }
            break;
        case SC_DATA:
            {
                slog(LOG_INFO, "httptask data(%d)", msg->msg.fd);
                ProcessHttpData(msg->msg.data, msg->msg.ud);
            }
            break;
        case SC_SEND: // send is asynchronous, only when received this msg, can we try to close the http connection if necessary.
            {
                slog(LOG_INFO, "httptask send done(%d)", msg->msg.fd);
                context_.HandleSendDone();
            }
            break;
        default:
            {
                // not interested in other events.
                // make sure http server will not throw them here.
                assert(0);
            }
    }
}

void HttpTask::Run()
{
    if (pthread_mutex_trylock(&lock_)) return;

    SocketEvent* msg;
    while (sockMsgQueue_.Pop((void**)&msg))
    {
        ProcessSocketMessage(msg);
        SocketServer::DefaultSockEventHandler(*msg);
        msgPool_.ReleaseBuffer(msg);
    }

    pthread_mutex_unlock(&lock_);
}


