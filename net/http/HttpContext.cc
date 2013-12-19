#include "HttpContext.h"

HttpContext::HttpContext(LockFreeBuffer& alloc)
    : keepalive_(false), curStage_(HS_INVALID)
    , buffer_(alloc)
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
    buffer_.ReleaseBuffer();
}

void HttpContext::AppendData(const char* data, size_t sz)
{
    buffer_.AppendData(data, sz);
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
        if (keepalive_ == false)
        {
            // TODO close connection
        }
    }
}

