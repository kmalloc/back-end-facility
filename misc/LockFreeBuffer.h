#ifndef __LOCK_FREE_BUFFER_H_
#define __LOCK_FREE_BUFFER_H_

#include "LockFreeContainer.h"

class LockFreeBuffer
{
    public:

        LockFreeBuffer(int bufsz, int poolsz);
        ~LockFreeBuffer();

    private:

        volatile int m_free;

        const int m_bufSz;
        const int m_poolSz;

        LockFreeStack<void*> m_free;
        LockFreeStack<void*> m_used;
};

#endif

