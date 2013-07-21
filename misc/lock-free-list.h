#ifndef _MISC_LOCk_FREE_LIST_H_
#define _MISC_LOCk_FREE_LIST_H_

#include "sys/atomic_ops.h"
#include "misc/PerThreadMemory.h"


class ListQueue
{
    public:

        ListQueue(size_t capacity = 0);
        ~ListQueue();

        bool Push(void* data);
        bool Pop(void*& data);
        bool Pop(void** data);
        size_t Size() const { return m_no; }
        bool IsEmpty() const { return m_no == 0; }

    private:

        struct LockFreeListNode;

        LockFreeListNode* AllocNode();
        void ReleaseNode(LockFreeListNode*);

        volatile size_t m_id;
        size_t    m_no;
        const size_t m_max;
        DoublePointer m_in;
        DoublePointer m_out;

        PerThreadMemoryAlloc m_alloc;
};


#endif

