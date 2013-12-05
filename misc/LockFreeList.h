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
        size_t Size() const { return no_; }
        bool IsEmpty() const { return no_ == 0; }

    private:

        struct LockFreeListNode;

        LockFreeListNode* AllocNode();
        void ReleaseNode(LockFreeListNode*);
        void InitInternalNodeList();

        volatile size_t id_;
        size_t    no_;
        const size_t max_;
        DoublePointer in_;
        DoublePointer out_;

        volatile size_t id2_;
        DoublePointer head_; // point to unused LockFreeListNode list
        LockFreeListNode* freeList_; //internal node list
};


#endif

