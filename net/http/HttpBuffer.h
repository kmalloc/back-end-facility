#ifndef __HTTP_BUFFER_H__
#define __HTTP_BUFFER_H__

#include <stdlib.h>
#include "misc/NonCopyable.h"

class HttpBuffer: public noncopyable
{
    public:

        HttpBuffer();
        ~HttpBuffer();

        const char* Get(size_t offset = 0) const;
        const char* GetStart() const;
        const char* GetEnd() const;

        void Consume(size_t sz);
        void ReleaseBuffer();
        void SetBuffer(char* buff, short off, short size);

        short MoveData();

        static const char CTRL[];
        static const char HEADER_DELIM[];

    private:

        int cur_;
        int end_;

        char* buff_; // allocate from lockfree buffer pool
};

#endif

