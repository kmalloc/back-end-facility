#include "HttpBuffer.h"

#include <string.h>
#include <assert.h>

const char HttpBuffer::CTRL[] = "\r\n";

HttpBuffer::HttpBuffer(LockFreeBuffer& alloc)
    : buff_(NULL), size_(alloc.Granularity())
    , start_(0), end_(0)
    , cur_(0), bufferAlloc_(alloc)
{
}

HttpBuffer::~HttpBuffer()
{
    ReleaseBuffer();
}

bool HttpBuffer::ResetBuffer()
{
    if (buff_) bufferAlloc_.ReleaseBuffer(buff_);

    start_ = end_ = cur_ = 0;
    buff_ = bufferAlloc_.AllocBuffer();

    if (buff_)
    {
        memset(buff_, 0, size_);
    }

    return buff_ != NULL;
}

void HttpBuffer::ReleaseBuffer()
{
    if (buff_) bufferAlloc_.ReleaseBuffer(buff_);

    buff_ = NULL;
    start_ = end_ = cur_ = 0;
}

void HttpBuffer::Consume(size_t sz)
{
    cur_ += sz;
    cur_ = (cur_ >= end_)? end_ : cur_;

    if (cur_ == end_)
    {
        memset(buff_, 0, end_);
        start_ = cur_ = end_ = 0;
    }
}

const char* HttpBuffer::Get(size_t off) const
{
    assert(buff_);

    if (cur_ + off >= end_) return NULL;

    return buff_ + start_ + cur_ + off;
}

const char* HttpBuffer::GetStart() const
{
    return buff_ + cur_;
}

const char* HttpBuffer::GetEnd() const
{
    return buff_ + end_;
}

size_t HttpBuffer::Append(const char* data, size_t sz)
{
    assert(buff_);

    if (buff_ + end_ + sz >= buff_ + size_)
    {
        size_t left = size_ - (end_ - start_);
        sz = (sz > left)? left : sz;

        memmove(buff_, buff_ + start_, end_ - start_);

        cur_ = cur_ - start_;
        end_ = end_ - start_;
        start_ = 0;

        memset(buff_ + end_, 0, size_ - end_);
    }

    memcpy(buff_ + end_, data, sz);
    end_ += sz;

    return sz;
}

