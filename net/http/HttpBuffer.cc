#include "HttpReadBuffer.h"

#include <string.h>
#include <assert.h>

const char CTRL[] = "\r\n";
const char HEADER_DELIM[] = ": ";

HttpReadBuffer::HttpReadBuffer()
    : readBuff_(NULL)
{
    InitHttpReadBuffer();
}

HttpReadBuffer::~HttpReadBuffer()
{
    FreeBuffer();
}

void HttpReadBuffer::InitBuffer()
{
    readBuff_ = SocketBufferList::AllocNode(4*1024);
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

void HttpReadBuffer::ReleaseBuffer(int sz)
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

void HttpReadBuffer::ConsumeRead(int sz)
{
    int left = (readBuff_->memFrame_ + readBuff_->size_) - (readBuff_->curPtr_ + readBuff_->curSize_);
    assert(sz <= left);

    readBuff_->curSize_ += sz;
}

const char* HttpReadBuffer::GetReadPoint(int off) const
{
    if (off >= readBuff_->curSize_) return NULL;

    return readBuff_->curPtr_ + off;
}

const char* HttpReadBuffer::GetReadStart() const
{
    return readBuff_->curPtr_;
}

const char* HttpReadBuffer::GetReadEnd() const
{
    return readBuff_->curPtr_ + readBuff_->curSize_;
}

short HttpReadBuffer::MoveDataToFront(SocketBufferNode* node) const
{
    memmove(node->memFrame_, node->curPtr_, node->curSize_);

    node->curPtr_ = node->memFrame_;
    return node->curSize_;
}

void HttpReadBuffer::SetExpandReadBuffer(int sz)
{
    readBuff_->curSize_ += sz;
    assert(readBuff_->curPtr_ + readBuff_->curSize_ <= readBuff_->memFrame_ + readBuff_->size_);
}

HttpReadBufferEntity HttpReadBuffer::GetFreeBuffer()
{
    HttpReadBufferEntity entity;
    int left = readBuff_->size_ - (readBuff_->curPtr_ - readBuff_->memFrame_) - readBuff_->curSize_;

    if (left < MINI_SOCKET_READ_SIZE) MoveDataToFront(readBuff_);

    entity.buff = readBuff_->curPtr_ + readBuff_->curSize_;
    entity.size = readBuff_->memFrame_ + readBuff_->size_ - (readBuff_->curPtr_ + readBuff_->curSize_);

    return entity;
}

// HttpWriteBuffer
HttpWriteBuffer::HttpWriteBuffer(int granularity, int num)
    :size_(granularity), num_(num)
    ,pendingBuffer_(NULL)
{
    InitBuffer();
}

HttpReadBufferEntity* HttpWriteBuffer::AllocWriteBuffer(int sz)
{
    if (sz <= 0 || sz > 4*size) return NULL;

    int mod = sz%size_;

    mod = mod > 0? size_ - mod : 0;
    int index = (sz + mod)/size_ - 1;

    HttpReadBufferEntity* entity = NULL;
    HttpReadBufferEntity* list = freeBuffer_[index];

    if (list == NULL)
    {
        char* buff = (char*)malloc(size_*(index + 1));
        if (buff == NULL) return NULL;

        entity = (HttpReadBufferEntity*)malloc(sizeof(HttpReadBufferEntity));
        if (entity == NULL)
        {
            free(buff);
            return NULL;
        }

        entity->buff = buff;
        entity->size = size_*(index + 1);

        return entity;
    }

    entity = list;
    list   = list->next;
    entity->next = NULL;

    return entity;
}

HttpReadBufferEntity* HttpWriteBuffer::GetPendingWrite()
{
    HttpReadBufferEntity* ret = pendingBuffer_;

    if (pendingBuffer_) pendingBuffer_->next;

    return ret;
}

void HttpReadBuffer::ReleaseWriteBuffer(HttpReadBufferEntity* buf)
{
    int mod = buf->size%size_;
    mod = mod > 0? size_ - mod : 0;
    int index = (buf->size + mod)/size_ - 1;

    buf->next = freeBuffer_[index];
    freeBuffer_[index] = buf;
}

