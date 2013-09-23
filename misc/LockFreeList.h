#ifndef _MISC_LOCk_FREE_LIST_H_
#define _MISC_LOCk_FREE_LIST_H_

#include "sys/AtomicOps.h"
#include "misc/PerThreadMemory.h"
#include "misc/NonCopyable.h"

class LockFreeListQueue: public noncopyable
{
    public:

        LockFreeListQueue(size_t capacity = 2048);
        ~LockFreeListQueue();

        bool Push(void* data);
        bool Pop(void*& data);
        bool Pop(void** data);
        size_t Size() const { return m_no; }
        bool IsEmpty() const { return m_no == 0; }

    private:

        struct LockFreeListNode;

        LockFreeListNode* AllocNode();
        void ReleaseNode(LockFreeListNode*);
        void InitInternalNodeList();

        volatile size_t m_id;
        size_t    m_no;
        const size_t m_max;
        DoublePointer m_in;
        DoublePointer m_out;

        volatile size_t m_id2;
        DoublePointer m_head;
        LockFreeListNode* m_freeList; //internal node list
};


#endif

