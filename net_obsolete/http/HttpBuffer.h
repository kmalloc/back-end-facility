#ifndef __HTTP_BUFFER_H__
#define __HTTP_BUFFER_H__

#include <stdlib.h>
#include "misc/NonCopyable.h"
#include "net/SocketBuffer.h"

extern const char HTTP_CTRL[];
extern const char HTTP_HEADER_DELIM[];

struct HttpBufferEntity
{
    char* buffer;
    int   size;
};

class HttpReadBuffer: public noncopyable
{
    public:

        HttpReadBuffer(int size = 8*1024);
        ~HttpReadBuffer();

        const char* GetContentPoint(int offset = 0) const;
        const char* GetContentStart() const;
        const char* GetContentEnd() const;

        void ResetBuffer();
        void ConsumeBuffer(int sz);

        void IncreaseContentRange(int sz);
        HttpBufferEntity GetAvailableReadRange();

    private:

        void  FreeBuffer();
        void  InitBuffer();
        short MoveDataToFront(SocketBufferNode*) const;

        const int size_;
        SocketBufferNode* readBuff_;
};

class HttpWriteBuffer: public noncopyable
{
    public:

        HttpWriteBuffer(int granularity = 1024, int total = 8);
        ~HttpWriteBuffer();

        SocketBufferNode* AllocWriteBuffer(int sz);
        void ReleaseWriteBuffer(SocketBufferNode* entity);

    private:

        bool  InitBuffer();
        void  DestroyBuffer();

        const int size_;
        const int num_;
        const int num_slot_;

        SocketBufferNode** freeBuffer_;
};

#endif

