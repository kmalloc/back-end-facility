#ifndef __HTTP_BUFFER_H__
#define __HTTP_BUFFER_H__

#include <stdlib.h>
#include "misc/NonCopyable.h"
#include "misc/LockFreeBuffer.h"

class HttpBuffer: public noncopyable 
{
    public:

        HttpBuffer(LockFreeBuffer& alloc);
        ~HttpBuffer();

        void    Consume(size_t sz);
        size_t  Append(const char* data, size_t sz);

        const char* Get(size_t offset = 0) const;
        const char* GetStart() const;
        const char* GetEnd() const;

        bool ResetBuffer();
        void ReleaseBuffer();

        static const char CTRL[];
        static const char HEADER_DELIM[];

    private:

        char* buff_; // allocate from lockfree buffer pool
        size_t size_;
        int start_;
        int end_;
        int cur_;
        LockFreeBuffer& bufferAlloc_;
};

#endif

