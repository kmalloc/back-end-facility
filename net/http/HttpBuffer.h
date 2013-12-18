#ifndef __HTTP_BUFFER_H__
#define __HTTP_BUFFER_H__

#include <stdlib.h>
#include "misc/NonCopyable.h"
#include "misc/LockFreeBuffer.h"

class HttpBuffer:public NonCopyable
{
    public:

        HttpBuffer(bufferAlloc_& alloc);
        ~HttpBuffer();

        void  Consume(int sz);
        void  Append(const char* data, size_t sz);

        char* Get(size_t offset = 0) const;

        bool ResetBuffer();
        void ReleaseBuffer();

    private:

        char* buff_; // allocate from lockfree buffer pool
        size_t size_;
        int start_;
        int end_;
        int cur_;
        LockFreeBuffer& bufferAlloc_;
};

#endif

