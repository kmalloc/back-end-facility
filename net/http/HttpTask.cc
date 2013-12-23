#include "HttpTask.h"

#include "net/http/HttpRequest.h"
#include "net/http/HttpRequest.h"

#include <map>
#include <string>

static void DefaultHttpRequestHandler(const HttpRequest& req, HttpResponse& response)
{
    response.SetShouldResponse(true);
    response.SetStatusCode(HttpResponse::HSC_200);
    response.SetStatusMessage("from miliao http server, default generated message");

    using namespace std;

    const map<string, string>& headers = req.GetHeader();
    map<string, string>::const_iterator it = headers.begin();

    string body ;
    body.reserve(128);

    body = "auto generated from request header:";
    while (it != headers.end())
    {
        body += it->first;
        body += ":";
        body += it->second;
        body += "\n";

        ++it;
    }

    body += "body from request:" + req.GetHttpBody();

    response.SetBody(body.c_str());

    char bodylen[32] = {0};
    snprintf(bodylen, 32, "%d", body.size());

    response.AddHeader("Content-Type", "text/html;charset=UTF-8");
    response.AddHeader("Content-Length", bodylen);

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

bool HttpTask::PostSockMsg(ConnMessage* msg)
{
    return sockMsgQueue_.Push(msg);
}

void HttpTask::ClearMsgQueue()
{
    ConnMessage* msg;
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

void HttpTask::ProcessSocketMessage(ConnMessage* msg)
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
    ConnMessage* msg;
    while (sockMsgQueue_.Pop((void**)&msg))
    {
        if (taskClosed_ == false) ProcessSocketMessage(msg);

        httpServer_->ReleaseSockMsg(msg);
    }
}


