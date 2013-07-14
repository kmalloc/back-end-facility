#ifndef _MISC_LOCk_FREE_LIST_H_
#define _MISC_LOCk_FREE_LIST_H_

#include "sys/atomic_ops.h"
#include "misc/PerThreadMemory.h"

struct ListNode;

class ListQueue
{
    public:

        ListQueue();
        ~ListQueue();

        bool Push(void* data);
        bool Pop(void*& data);

    private:

        ListNode* AllocNode();
        void ReleaseNode(ListNode*);

        size_t    m_id;
        DoublePointer m_in;
        DoublePointer m_out;

        PerThreadMemoryAlloc m_alloc;
};


#endif

