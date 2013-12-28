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

void HttpBuffer::ReleaseBuffer()
{
    buff_ = NULL;
    end_ = cur_ = 0;
}

void HttpBuffer::SetBuffer(char* buff, short off, short size)
{
    cur_ = (buff_ == buff)? cur_ : off;

    buff_ = buff;
    end_  = off + size;
}

void HttpBuffer::Consume(size_t sz)
{
    cur_ += sz;
    cur_ = (cur_ >= end_)? end_ : cur_;
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

short HttpBuffer::MoveData()
{
    assert(buff_);

    short sz = end_ - cur_;
    memmove(buff_, buff_ + cur_, sz);

    cur_ = 0;
    end_ = sz;

    return sz;
}

