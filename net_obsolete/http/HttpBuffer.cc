#include "HttpBuffer.h"

#include <string.h>
#include <assert.h>

const char HTTP_CTRL[] = "\r\n";
const char HTTP_HEADER_DELIM[] = ": ";

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
    readBuff_ = SocketBufferList::AllocNode(size_);
}

void HttpReadBuffer::FreeBuffer()
{
    SocketBufferList::FreeBufferNode(readBuff_);
}

void HttpReadBuffer::ResetBuffer()
{
    readBuff_->curPtr_ = readBuff_->memFrame_;
    readBuff_->curSize_ = 0;
}

void HttpReadBuffer::ConsumeBuffer(int sz)
{
    if (sz > readBuff_->curSize_) sz = readBuff_->curSize_;

    if (sz == readBuff_->curSize_)
    {
        readBuff_->curPtr_ = readBuff_->memFrame_;
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

short HttpReadBuffer::MoveDataToFront(SocketBufferNode* node) const
{
    memmove(node->memFrame_, node->curPtr_, node->curSize_);

    node->curPtr_ = node->memFrame_;
    return node->curSize_;
}

void HttpReadBuffer::IncreaseContentRange(int sz)
{
    readBuff_->curSize_ += sz;
    assert(readBuff_->curPtr_ + readBuff_->curSize_ <= readBuff_->memFrame_ + readBuff_->size_);
}

HttpBufferEntity HttpReadBuffer::GetAvailableReadRange()
{
    HttpBufferEntity entity;
    int left = readBuff_->size_ - (readBuff_->curPtr_ - readBuff_->memFrame_) - readBuff_->curSize_;

    if (left < MINI_SOCKET_READ_SIZE) MoveDataToFront(readBuff_);

    entity.buffer = readBuff_->curPtr_ + readBuff_->curSize_;
    entity.size   = readBuff_->memFrame_ + readBuff_->size_ - (readBuff_->curPtr_ + readBuff_->curSize_);

    return entity;
}

// HttpWriteBuffer
HttpWriteBuffer::HttpWriteBuffer(int granularity, int num)
    :size_(granularity), num_(num)
    ,num_slot_(4)
{
    assert(InitBuffer());
}

HttpWriteBuffer::~HttpWriteBuffer()
{
    DestroyBuffer();
}

bool HttpWriteBuffer::InitBuffer()
{
    freeBuffer_ = (SocketBufferNode**)malloc(num_slot_*sizeof(SocketBufferNode*));
    for (int i = 0; i < num_slot_; ++i)
    {
        freeBuffer_[i] = NULL;
    }

    for (int i = 0; i < num_; ++i)
    {
        SocketBufferNode* entity = (SocketBufferNode*)malloc(sizeof(SocketBufferNode) + size_);
        if (entity == NULL) return false;

        entity->size_    = size_;
        entity->next_    = freeBuffer_[0];
        entity->curPtr_  = entity->memFrame_;
        entity->curSize_ = 0;
        freeBuffer_[0]   = entity;
    }

    return true;
}

void HttpWriteBuffer::DestroyBuffer()
{
    int i = 0;
    while (i < num_slot_)
    {
        SocketBufferNode* cur = freeBuffer_[i];
        SocketBufferNode* next;

        while (cur)
        {
            next = cur->next_;
            free(cur);
            cur = next;
        }

        freeBuffer_[i] = NULL;
        ++i;
    }

    free(freeBuffer_);
}

SocketBufferNode* HttpWriteBuffer::AllocWriteBuffer(int sz)
{
    if (sz <= 0 || sz > num_slot_*size_) return NULL;

    int mod = sz%size_;

    mod = mod > 0? size_ - mod : 0;
    int index = (sz + mod)/size_ - 1;

    SocketBufferNode* entity = NULL;
    SocketBufferNode* list = freeBuffer_[index];

    if (list == NULL)
    {
        entity = (SocketBufferNode*)malloc(sizeof(SocketBufferNode) + (index + 1)*size_);
        if (entity == NULL) return NULL;

        entity->size_    = size_*(index + 1);
        entity->curPtr_  = entity->memFrame_;
        entity->curSize_ = 0;
        entity->next_    = NULL;

        return entity;
    }

    entity = list;
    freeBuffer_[index] = list->next_;

    entity->next_ = NULL;
    entity->curPtr_  = entity->memFrame_;
    entity->curSize_ = 0;

    return entity;
}

void HttpWriteBuffer::ReleaseWriteBuffer(SocketBufferNode* buf)
{
    int mod = buf->size_%size_;
    mod = mod > 0? size_ - mod : 0;
    int index = (buf->size_ + mod)/size_ - 1;

    buf->next_ = freeBuffer_[index];
    buf->curPtr_  = buf->memFrame_;
    buf->curSize_ = 0;

    freeBuffer_[index] = buf;
}

