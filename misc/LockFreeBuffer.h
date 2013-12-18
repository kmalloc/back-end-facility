#ifndef __LOCK_FREE_BUFFER_H_
#define __LOCK_FREE_BUFFER_H_

#include "sys/AtomicOps.h"
#include "misc/NonCopyable.h"

struct BufferNode;

class LockFreeBuffer: public noncopyable
{
    public:

        LockFreeBuffer(size_t size, size_t granularity);
        ~LockFreeBuffer();

        char* AllocBuffer();
        void  ReleaseBuffer(void* buffer);

        size_t Granularity() const { return granularity_; }
        size_t Capacity() const { return size_; }

    private:

        void InitList();

        const size_t size_; // total number of buffer.
        const size_t granularity_;

        volatile size_t id_;
        char* rawBuff_;
        DoublePointer head_;
        BufferNode** freeList_;
};

#endif

