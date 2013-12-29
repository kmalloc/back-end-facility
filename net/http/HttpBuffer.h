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

        const char* GetRead(size_t offset = 0) const;
        const char* GetReadStart() const;
        const char* GetReadEnd() const;

        void ConsumeRead(size_t sz);
        void ReleaseReadBuffer();
        void ReleaseWriteBuffer();

        bool GetReadBuffer(HttpBufferEntity& entity);
        bool GetWriteBuffer(HttpBufferEntity& entity);

        short MoveReadDataToFront();

        static const char CTRL[];
        static const char HEADER_DELIM[];

    private:

        int curRead_;
        int endRead_;

        SocketBufferList readList_;
        SocketBufferList writeList_;
};

#endif

