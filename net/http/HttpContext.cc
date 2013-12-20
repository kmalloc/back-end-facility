#include "HttpContext.h"

#include "sys/Log.h"

HttpContext::HttpContext(LockFreeBuffer& alloc, HttpCallBack cb)
    : keepalive_(false), curStage_(HS_INVALID)
    , buffer_(alloc), callBack_(cb)
{
}

HttpContext::~HttpContext()
{
    ReleaseContext();
}

void HttpContext::ResetContext(bool keepalive)
{
    keepalive_ = keepalive;
    curStage_ = HS_REQUEST_LINE;

    buffer_.ResetBuffer();
}

void HttpContext::ReleaseContext()
{
    request_.CleanUp();
    response_.CleanUp();

    buffer_.ReleaseBuffer();
}

void HttpContext::AppendData(const char* data, size_t sz)
{
    size_t size = buffer_.AppendData(data, sz);

    if (size < sz)
    {
        slog(LOG_WARN, "http request, data lost");
    }
}

// in normal case, this should be called after respone is done.
void HttpContext::CleanUp()
{
    request_.CleanUp();
    response_.CleanUp();

    if (keepalive_ == false)
    {
        conn_.CloseConnection();
    }
}

void HttpContext::DoResponse()
{
    std::string result = callBack_(httpReqLine_);
    if (!result.empty())
    {
        conn_.SendData(result.c_str(), result.size() + 1);
    }
}

void HttpContext::RunParser()
{
    if (ShouldParseRequestLine())
    {
        ParseRequestLine();
        FinishParsingRequestLine();
    }
    else if (ShouldParseHeader())
    {
        ParseHeader();
        FinishParsingHeader();
    }
    else if (ShouldParseBody())
    {
        ParseBody();
        FinishParsingBody();
    }
    else
    {
        DoResponse();
    }
}

void HttpContext::ParseRequestLine()
{

}

