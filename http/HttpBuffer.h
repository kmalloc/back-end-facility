#ifndef __HTTP_BUFFER_H__
#define __HTTP_BUFFER_H__

#include <stdlib.h>
#include "misc/NonCopyable.h"

struct HttpBuffer
{
    int size_;
    int curSize_;
    char* curPtr_;
    HttpBuffer* next_;

    char memory_[1];
};

class HttpBufferList
{
    public:

        HttpBufferList();
        ~HttpBufferList();

        HttpBuffer* GetFront() const;
        HttpBuffer* PopFront();
        HttpBuffer* GetTail() const;

        void PushBack(HttpBuffer* buf);

    private:

        HttpBuffer* head_;
        HttpBuffer* tail_;
};

class HttpReadBuffer: public noncopyable
{
    public:

        explicit HttpReadBuffer(int size = 8*1024);
        ~HttpReadBuffer();

        int GetContenLen() const;
        const char* GetContentPoint(int offset = 0) const;
        const char* GetContentStart() const;
        const char* GetContentEnd() const;

        void ResetBuffer();
        void ConsumeBuffer(int sz);

        void IncreaseContentRange(int sz);

        char* GetFreeBuffer(int& size);

    private:

        void  FreeBuffer();
        void  InitBuffer();
        short MoveDataToFront(HttpBuffer*) const;

        const int size_;
        HttpBuffer* readBuff_;
};

class HttpWriteBuffer: public noncopyable
{
    public:

        explicit HttpWriteBuffer(int granularity = 1024, int total = 8);
        ~HttpWriteBuffer();

        HttpBuffer* AllocWriteBuffer(int sz);
        void ReleaseWriteBuffer(HttpBuffer* entity);

    private:

        bool  InitBuffer();
        void  DestroyBuffer();

        const int size_;
        const int num_;
        const int num_slot_;

        HttpBuffer** freeBuffer_;
};

#endif

