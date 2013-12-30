#include "HttpBuffer.h"

#include <string.h>
#include <assert.h>

const char HttpBuffer::CTRL[] = "\r\n";
const char HttpBuffer::HEADER_DELIM[] = ": ";

HttpBuffer::HttpBuffer()
    : readBuff_(NULL)
    , writeBuff_(NULL)
{
    InitHttpBuffer();
}

HttpBuffer::~HttpBuffer()
{
    FreeReadWriteBuffer();
}

void HttpBuffer::InitHttpBuffer()
{
    int i = 0;
    const int node_size = 4*1024;

    SocketBufferNode* node = SocketBufferList::AllocNode(node_size);
    while (node && i++ < 4)
    {
        freeWriteBuffer_.AppendBufferNode(node);
        node = SocketBufferList::AllocNode(node_size);
    }

    writeBuff_ = freeWriteBuffer_.PopFrontNode();
    readBuff_ = SocketBufferList::AllocNode(node_size);
}

void HttpBuffer::FreeReadWriteBuffer()
{
    // TODO
}

void HttpBuffer::ResetReadBuffer()
{
    readBuff_->curPtr_ = readBuff_->memFrame_;
    readBuff_->curSize_ = 0;
}

void HttpBuffer::ResetWriteBuffer()
{
    if (writeBuff_ == NULL) writeBuff_ = writeList_.PopFrontNode();

    while (writeBuff_)
    {
        writeBuff_->curPtr_ = writeBuff_->memFrame_;
        writeBuff_->curSize_ = 0;
        freeWriteBuffer_.AppendBufferNode(writeBuff_);
        writeBuff_ = writeList_.PopFrontNode();
    }

    writeBuff_ = NULL;
}

void HttpBuffer::ReleaseReadBuffer(int sz)
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

void HttpBuffer::ConsumeRead(int sz)
{
    int left = (readBuff_->memFrame_ + readBuff_->size_) - (readBuff_->curPtr_ + readBuff_->curSize_);
    assert(sz <= left);

    readBuff_->curSize_ += sz;
}

void HttpBuffer::ConsumeWrite(int sz)
{
    int left = (writeBuff_->memFrame_ + writeBuff_->size_) - (writeBuff_->curPtr_ + writeBuff_->curSize_);
    if (sz > left) sz = left;

    writeBuff_->curSize_ += sz;

    if (sz == left)
    {
        writeList_.AppendBufferNode(writeBuff_);
        writeBuff_ = freeWriteBuffer_.PopFrontNode();

        writeBuff_->curSize_ = 0;
        writeBuff_->curPtr_ = writeBuff_->memFrame_;
    }
}

const char* HttpBuffer::GetReadPoint(int off) const
{
    if (off >= readBuff_->curSize_) return NULL;

    return readBuff_->curPtr_ + off;
}

const char* HttpBuffer::GetReadStart() const
{
    return readBuff_->curPtr_;
}

const char* HttpBuffer::GetReadEnd() const
{
    return readBuff_->curPtr_ + readBuff_->curSize_;
}

short HttpBuffer::MoveDataToFront(SocketBufferNode* node) const
{
    memmove(node->memFrame_, node->curPtr_, node->curSize_);

    node->curPtr_ = node->memFrame_;
    return node->curSize_;
}

void HttpBuffer::GetFreeReadBuffer(HttpBufferEntity& entity)
{
    int left = readBuff_->size_ - (readBuff_->curPtr_ - readBuff_->memFrame_) - readBuff_->curSize_;

    if (left < MINI_SOCKET_READ_SIZE) MoveDataToFront(readBuff_);

    entity.buff = readBuff_->curPtr_ + readBuff_->curSize_;
    entity.size = readBuff_->memFrame_ + readBuff_->size_ - (readBuff_->curPtr_ + readBuff_->curSize_);
}

void HttpBuffer::GetFreeWriteBuffer(int sz, HttpBufferEntity& entity)
{
    if (writeBuff_ == NULL) writeBuff_ = freeWriteBuffer_.PopFrontNode();

    if (writeBuff_ == NULL || sz > writeBuff_->size_) return;

    int left = writeBuff_->memFrame_ + writeBuff_->size_ - (writeBuff_->curPtr_ + writeBuff_->curSize_);

    if (left < sz)
    {
        SocketBufferNode* node = freeWriteBuffer_.PopFrontNode();
        if (node == NULL)
        {
            MoveDataToFront(writeBuff_);
            if (writeBuff_->size_ - writeBuff_->curSize_ < sz) return;
        }
        else
        {
            writeList_.AppendBufferNode(writeBuff_);
            writeBuff_ = node;
        }
    }

    left = writeBuff_->memFrame_ + writeBuff_->size_ - (writeBuff_->curPtr_ + writeBuff_->curSize_);
    entity.buff = writeBuff_->curPtr_ + writeBuff_->curSize_;
    entity.size = left;
}

void HttpBuffer::GetPendingWrite(HttpBufferEntity& entity)
{
    if (pendingWriteBuff_ == NULL) pendingWriteBuff_ = writeList_.PopFrontNode();

    if (pendingWriteBuff_ == NULL) pendingWriteBuff_ = writeBuff_;

    entity.buff = pendingWriteBuff_->curPtr_;
    entity.size = pendingWriteBuff_->curSize_;
}

void HttpBuffer::ReleaseWriteBuffer(int sz)
{
    assert(sz <= pendingWriteBuff_->curSize_);
    pendingWriteBuff_->curPtr_ += sz;
    pendingWriteBuff_->curSize_ -= sz;

    if (pendingWriteBuff_->curSize_ == 0)
    {
        freeWriteBuffer_.AppendBufferNode(pendingWriteBuff_);
        pendingWriteBuff_ = NULL;
    }
}

