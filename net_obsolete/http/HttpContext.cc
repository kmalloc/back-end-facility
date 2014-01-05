#include "HttpContext.h"
#include "sys/Log.h"

#include <stdlib.h>
#include <string>

HttpContext::HttpContext(SocketServer& server, HttpCallBack cb)
    : conn_(server), httpReader_(conn_), httpWriter_(conn_)
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
    conn_.ResetConnection(connid);
}

// call this function only when connection is already closed
void HttpContext::ReleaseContext()
{
    httpWriter_.ResetWriter();
    httpReader_.ResetReader();
    conn_.ReleaseConnection();
}

void HttpContext::ForceCloseConnection()
{
    httpReader_.ResetReader();
    httpWriter_.ResetWriter();

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
            sz = httpWriter_.SendPendingBuffer();

            if (sz < 0) ForceCloseConnection();
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

