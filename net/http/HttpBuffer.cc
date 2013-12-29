#include "HttpBuffer.h"

#include <string.h>
#include <assert.h>

const char HttpBuffer::CTRL[] = "\r\n";
const char HttpBuffer::HEADER_DELIM[] = ": ";

HttpBuffer::HttpBuffer()
    : cur_(0), end_(0)
    , buff_(NULL)
{
}

HttpBuffer::~HttpBuffer()
{
    ReleaseBuffer();
}

void HttpBuffer::ReleaseReadBuffer()
{
    endRead_ = curRead_ = 0;
}

void HttpBuffer::MoveReadEndCursor(int off)
{
    endRead_ += off;
}

void HttpBuffer::ConsumeRead(size_t sz)
{
    curRead_ += sz;
    curRead_ = (curRead_ >= endRead_)? endRead_ : curRead_;
}

const char* HttpBuffer::Get(size_t off) const
{
    if (cur_ + off >= end_) return NULL;

    return buff_ + cur_ + off;
}

const char* HttpBuffer::GetStart() const
{
    return buff_ + cur_;
}

const char* HttpBuffer::GetEnd() const
{
    return buff_ + end_;
}

short HttpBuffer::MoveDataToFront()
{
    assert(buff_);

    short sz = end_ - cur_;
    memmove(buff_, buff_ + cur_, sz);

    cur_ = 0;
    end_ = sz;

    return sz;
}

