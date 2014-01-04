#include "HttpBuffer.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

static HttpBuffer* AllocHttpBuffer(int sz)
{
    HttpBuffer* buff = (HttpBuffer*)malloc(sizeof(HttpBuffer) + sz);
    if (buff == NULL) return NULL;

    buff->size_ = sz;
    buff->curPtr_ = buff->memory_;
    buff->curSize_ = 0;
    buff->next_ = NULL;

    return buff;
}

static inline void FreeHttpBuffer(HttpBuffer* buf)
{
    free(buf);
}

HttpBufferList::HttpBufferList()
    :head_(NULL), tail_(NULL)
{
}

HttpBufferList::~HttpBufferList()
{
}

HttpBuffer* HttpBufferList::PopFront()
{
    HttpBuffer* ret = head_;

    if (head_) head_ = head_->next_;

    if (head_ == NULL) tail_ = NULL;

    return ret;
}

HttpBuffer* HttpBufferList::GetFront() const
{
    return head_;
}

HttpBuffer* HttpBufferList::GetTail() const
{
    return tail_;
}

void HttpBufferList::PushBack(HttpBuffer* node)
{
    assert(node);

    if (tail_) tail_->next_ = node;

    if (head_ == NULL) head_ = node;

    tail_ = node;
}

// HttpReadBuffer
HttpReadBuffer::HttpReadBuffer(int size)
    : readBuff_(NULL)
    , size_(size)
{
    InitBuffer();
}

HttpReadBuffer::~HttpReadBuffer()
{
    FreeBuffer();
}

void HttpReadBuffer::InitBuffer()
{
    readBuff_ = AllocHttpBuffer(size_);
}

void HttpReadBuffer::FreeBuffer()
{
    FreeHttpBuffer(readBuff_);
}

void HttpReadBuffer::ResetBuffer()
{
    readBuff_->curPtr_ = readBuff_->memory_;
    readBuff_->curSize_ = 0;
}

void HttpReadBuffer::ConsumeBuffer(int sz)
{
    if (sz > readBuff_->curSize_) sz = readBuff_->curSize_;

    if (sz == readBuff_->curSize_)
    {
        readBuff_->curPtr_ = readBuff_->memory_;
    }
    else
    {
        readBuff_->curPtr_ += sz;
    }

    readBuff_->curSize_ -= sz;
}

const char* HttpReadBuffer::GetContentPoint(int off) const
{
    if (off >= readBuff_->curSize_) return NULL;

    return readBuff_->curPtr_ + off;
}

const char* HttpReadBuffer::GetContentStart() const
{
    return readBuff_->curPtr_;
}

const char* HttpReadBuffer::GetContentEnd() const
{
    return readBuff_->curPtr_ + readBuff_->curSize_;
}

short HttpReadBuffer::MoveDataToFront(HttpBuffer* node) const
{
    memmove(node->memory_, node->curPtr_, node->curSize_);
    node->curPtr_ = node->memory_;
    return node->curSize_;
}

void HttpReadBuffer::IncreaseContentRange(int sz)
{
    readBuff_->curSize_ += sz;
    assert(readBuff_->curPtr_ + readBuff_->curSize_ <= readBuff_->memory_ + readBuff_->size_);
}

#define MINI_SOCKET_READ_SIZE (64)

char* HttpReadBuffer::GetFreeBuffer(int& size)
{
    int left = readBuff_->size_ - (readBuff_->curPtr_ - readBuff_->memory_) - readBuff_->curSize_;

    if (left < MINI_SOCKET_READ_SIZE) MoveDataToFront(readBuff_);

    char* buffer = readBuff_->curPtr_ + readBuff_->curSize_;

    size   = readBuff_->memory_ + readBuff_->size_ - (readBuff_->curPtr_ + readBuff_->curSize_);

    return buffer;
}

int HttpReadBuffer::GetContenLen() const
{
    return readBuff_->curSize_;
}

// HttpWriteBuffer
HttpWriteBuffer::HttpWriteBuffer(int granularity, int num)
    :size_(granularity), num_(num)
    ,num_slot_(8)
{
    assert(InitBuffer());
}

HttpWriteBuffer::~HttpWriteBuffer()
{
    DestroyBuffer();
}

bool HttpWriteBuffer::InitBuffer()
{
    freeBuffer_ = (HttpBuffer**)malloc(num_slot_*sizeof(HttpBuffer*));
    for (int i = 0; i < num_slot_; ++i)
    {
        freeBuffer_[i] = NULL;
    }

    for (int i = 0; i < num_; ++i)
    {
        HttpBuffer* entity = AllocHttpBuffer(size_);
        if (entity == NULL) return false;

        entity->next_    = freeBuffer_[0];
        freeBuffer_[0]   = entity;
    }

    return true;
}

void HttpWriteBuffer::DestroyBuffer()
{
    int i = 0;
    while (i < num_slot_)
    {
        HttpBuffer* cur = freeBuffer_[i];
        HttpBuffer* next;

        while (cur)
        {
            next = cur->next_;
            FreeHttpBuffer(cur);
            cur = next;
        }

        freeBuffer_[i] = NULL;
        ++i;
    }

    free(freeBuffer_);
}

HttpBuffer* HttpWriteBuffer::AllocWriteBuffer(int sz)
{
    if (sz <= 0 || sz > num_slot_*size_) return NULL;

    int mod = sz%size_;

    mod = mod > 0? size_ - mod : 0;
    int index = (sz + mod)/size_ - 1;

    HttpBuffer* entity = NULL;
    HttpBuffer* list = freeBuffer_[index];

    if (list == NULL)
    {
        entity = AllocHttpBuffer((index + 1)*size_);
        return entity;
    }

    entity = list;
    freeBuffer_[index] = list->next_;

    entity->next_ = NULL;
    entity->curPtr_  = entity->memory_;
    entity->curSize_ = 0;

    return entity;
}

void HttpWriteBuffer::ReleaseWriteBuffer(HttpBuffer* buf)
{
    int mod = buf->size_%size_;

    mod = mod > 0? size_ - mod : 0;

    int index = (buf->size_ + mod)/size_ - 1;

    buf->next_ = freeBuffer_[index];
    buf->curPtr_  = buf->memory_;
    buf->curSize_ = 0;

    freeBuffer_[index] = buf;
}

