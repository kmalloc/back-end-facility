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

