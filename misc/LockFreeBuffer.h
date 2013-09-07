#ifndef __LOCK_FREE_BUFFER_H_
#define __LOCK_FREE_BUFFER_H_

#include "sys/atomic_ops.h"

class BufferNode;

class LockFreeBuffer
{
    public:

        LockFreeBuffer(size_t size, size_t granularity);
        ~LockFreeBuffer();

        char* AllocBuffer();
        void  ReleaseBuffer(void* buffer);

    private:

        void InitList();

        const int m_size; // total number of buffer.
        const int m_granularity;

        volatile size_t m_id;
        char* m_rawBuff;
        DoublePointer m_head;
        BufferNode** m_freeList;
};

#endif

