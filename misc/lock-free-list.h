#ifndef _MISC_LOCk_FREE_LIST_H_
#define _MISC_LOCk_FREE_LIST_H_

#include "misc/PerThreadMemory.h"

struct ListNode;

struct ListQueue
{
    public:

        ListQueue();
        ~ListQueue();

        bool Push(void* data);
        bool Pop(void*& data);

    private:

        ListNode* AllocNode();
        void ReleaseNode(ListNode*);

        size_t    m_no;
        ListNode* m_in;
        ListNode* m_out;

        PerThreadMemoryAlloc m_alloc;
};


#endif

