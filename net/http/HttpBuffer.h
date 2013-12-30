#ifndef __HTTP_BUFFER_H__
#define __HTTP_BUFFER_H__

#include <stdlib.h>
#include "misc/NonCopyable.h"
#include "net/SocketBuffer.h"

extern const char CTRL[];
extern const char HEADER_DELIM[];

struct HttpBufferEntity
{
    char* buff;
    int   size;

    HttpBufferEntity* next;
};

class HttpReadBuffer: public noncopyable
{
    public:

        HttpBuffer();
        ~HttpBuffer();

        const char* GetReadPoint(int offset = 0) const;
        const char* GetReadStart() const;
        const char* GetReadEnd() const;

        void ResetBuffer();

        void ConsumeRead(int sz);
        void ReleaseBuffer(int sz);

        int  SetExpandReadBuffer(int sz);
        HttpBufferEntity GetFreeBuffer();

    private:

        void  FreeBuffer();
        void  InitBuffer();
        short MoveDataToFront(SocketBufferNode*) const;

        SocketBufferNode* readBuff_;
};

class HttpWriteBuffer: public noncopyable
{
    public:

        HttpWriteBuffer(int granularity = 1024, int total = 8);
        ~HttpWriteBuffer();

        void ResetBuffer();
        void ReleaseBuffer(int sz);

        HttpBufferEntity* GetPendingWrite();
        HttpBufferEntity* AllocWriteBuffer(const int sz);

        void AddWriteBuffer(HttpBufferEntity* entity);
        void ReleaseWriteBuffer(HttpBufferEntity* entity);

    private:

        void  DestroyAllBuffer();
        void  InitHttpBuffer();

        const int size_;
        const int num_;
        HttpBufferEntity* pendingBuffer_;
        HttpBufferEntity* freeBuffer_[4];
};
#endif

