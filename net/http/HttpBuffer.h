#ifndef __HTTP_BUFFER_H__
#define __HTTP_BUFFER_H__

#include <stdlib.h>
#include "misc/NonCopyable.h"
#include "net/SocketBuffer.h"

struct HttpBufferEntity
{
    char* buff;
    int   size;
};

class HttpBuffer: public noncopyable
{
    public:

        HttpBuffer();
        ~HttpBuffer();

        const char* GetReadPoint(int offset = 0) const;
        const char* GetReadStart() const;
        const char* GetReadEnd() const;

        void ResetReadBuffer();
        void ResetWriteBuffer();

        void ConsumeRead(int sz);
        void ConsumeWrite(int sz);
        void ReleaseReadBuffer(int sz);
        void ReleaseWriteBuffer(int sz);

        void GetFreeReadBuffer(HttpBufferEntity& entity);
        void GetFreeWriteBuffer(const int sz, HttpBufferEntity& entity);

        void GetPendingWrite(HttpBufferEntity& entity);

        static const char CTRL[];
        static const char HEADER_DELIM[];

    private:

        void FreeReadWriteBuffer();
        void InitHttpBuffer();
        short MoveDataToFront(SocketBufferNode*) const;

        SocketBufferNode* readBuff_;
        SocketBufferNode* writeBuff_;
        SocketBufferNode* pendingWriteBuff_;

        SocketBufferList writeList_;
        SocketBufferList freeWriteBuffer_;
};

#endif

