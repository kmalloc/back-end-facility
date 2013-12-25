#include "HttpTask.h"

#include "net/http/HttpRequest.h"
#include "net/http/HttpRequest.h"

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

    response.SetCloseConn(true);
}

HttpTask::HttpTask(HttpServer* server, LockFreeBuffer& alloc)
    : ITask(false)
    , taskClosed_(false)
    , httpServer_(server)
    , context_(*server, alloc, &DefaultHttpRequestHandler)
    , sockMsgQueue_(64)
{
}

HttpTask::~HttpTask()
{
    ReleaseTask();
}

void HttpTask::ReleaseTask()
{
    context_.ReleaseContext();
    ClearMsgQueue();
    taskClosed_ = true;
    SetAffinity(-1);
}

void HttpTask::CloseTask()
{
    taskClosed_ = true;
    ClearMsgQueue();
}

bool HttpTask::PostSockMsg(SocketEvent* msg)
{
    return sockMsgQueue_.Push(msg);
}

void HttpTask::ClearMsgQueue()
{
    SocketEvent* msg;
    while (sockMsgQueue_.Pop((void**)&msg))
    {
        httpServer_->ReleaseSockMsg(msg);
    }
}

void HttpTask::ResetTask(int connid)
{
    context_.ResetContext(connid);
    taskClosed_ = false;
}

void HttpTask::ProcessHttpData(const char* data, size_t sz)
{
    context_.AppendData(data, sz);
    context_.ProcessHttpRequest();
}

void HttpTask::ProcessSocketMessage(SocketEvent* msg)
{
    switch (msg->code)
    {
        case SC_DATA:
            {
                ProcessHttpData(msg->msg.data, msg->msg.ud);
            }
            break;
        case SC_SEND: // send is asynchronous, only when received this msg, can we try to close the http connection if necessary.
            {
                context_.HandleSendDone();
            }
            break;
        case SC_CLOSE:
            {
                ReleaseTask();
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
    SocketEvent* msg;
    while (sockMsgQueue_.Pop((void**)&msg))
    {
        if (taskClosed_ == false) ProcessSocketMessage(msg);

        httpServer_->ReleaseSockMsg(msg);
    }
}


