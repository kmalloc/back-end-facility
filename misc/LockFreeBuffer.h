#ifndef __LOCK_FREE_BUFFER_H_
#define __LOCK_FREE_BUFFER_H_

#include "sys/AtomicOps.h"

struct BufferNode;

class LockFreeBuffer
{
    public:

        LockFreeBuffer(size_t size, size_t granularity);
        ~LockFreeBuffer();

        char* AllocBuffer();
        void  ReleaseBuffer(void* buffer);

    private:

        void InitList();

        const size_t m_size; // total number of buffer.
        const size_t m_granularity;

        volatile size_t m_id;
        char* m_rawBuff;
        DoublePointer m_head;
        BufferNode** m_freeList;
};

#endif

