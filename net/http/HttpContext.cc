#include "HttpContext.h"
#include "sys/Log.h"

#include <stdlib.h>
#include <string>

HttpContext::HttpContext(SocketServer& server, HttpCallBack cb)
    : status_(HS_CLOSED)
    , conn_(server), httpReader_(conn_), httpWriter_(conn_)
    , callBack_(cb)
{
}

HttpContext::~HttpContext()
{
    ReleaseContext();
}

void HttpContext::ResetContext(int connid)
{
    ReleaseContext();

    status_ = HS_CONNECTED;
    httpWriter_.ResetWriter();
    httpReader_.ResetReader();
    conn_.ResetConnection(connid);
}

// call this function only when connection is already closed
void HttpContext::ReleaseContext()
{
    status_ = HS_CLOSED;

    httpWriter_.ResetWriter();
    httpReader_.ResetReader();
    conn_.ReleaseConnection();
}

void HttpContext::ForceCloseConnection()
{
    status_ = HS_CLOSING;
    conn_.CloseConnection();
}

void HttpContext::DoResponse(const HttpRequest& request)
{
    HttpResponse response;
    callBack_(request, response);

    if (response.ShouldCloseConnection())
    {
        ForceCloseConnection();
    }

    if (response.GetShouldResponse())
    {
        size_t sz = response.GetResponseSize() + 2;

        SocketBufferNode* node = httpWriter_.AllocWriteBuffer(sz);
        if (node == NULL)
        {
            ForceCloseConnection();
        }
        else
        {
            char* buff = node->memFrame_;
            sz = node->size_;
            sz = response.GenerateResponse(buff, sz);
            node->curSize_ = sz;

            httpWriter_.AddToWriteList(node);
            httpWriter_.SendPendingBuffer();
        }
    }

    httpReader_.CleanData();
}

int HttpContext::ProcessHttpWrite()
{
    return httpWriter_.SendPendingBuffer();
}

int HttpContext::ProcessHttpRead()
{
    if (status_ != HS_CONNECTED) return 0;

    int ret = httpReader_.ProcessHttpRead();
    if (ret < 0)
    {
        ForceCloseConnection();
        return ret;
    }

    if (httpReader_.ShouldResponse())
    {
        DoResponse(httpReader_.GetHttpRequest());
    }

    return ret;
}

