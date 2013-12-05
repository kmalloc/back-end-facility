#include "LockFreeBuffer.h"

#include "sys/Defs.h"

#include <assert.h>

struct BufferNode
{
    DoublePointer next_;
    char buffer_[0];
};

LockFreeBuffer::LockFreeBuffer(size_t size, size_t granularity)
    :size_(size), granularity_(AlignTo(granularity, sizeof(DoublePointer)))
    ,id_(0), rawBuff_(NULL), freeList_(NULL)
{
    size_t sz = size_ * (sizeof(BufferNode) + granularity_);

    try
    {
        rawBuff_ = new char[sz];
        freeList_ = new BufferNode*[size_];
    }
    catch (...)
    {
        if (rawBuff_) delete[] rawBuff_;
        if (freeList_) delete[] freeList_;

        throw "out of memory";
    }

    InitList();
}

LockFreeBuffer::~LockFreeBuffer()
{
    delete[] freeList_;
    delete[] rawBuff_;
}

void LockFreeBuffer::InitList()
{
    size_t i, off_set = 0;
    const size_t sz = sizeof(BufferNode) + granularity_;

    for (i = 0; i < size_; ++i)
    {
        off_set = i * sz;
        freeList_[i] = (BufferNode*)(rawBuff_ + off_set);
    }

    for (i = 0; i < size_ - 1; ++i)
    {
        freeList_[i]->next_.vals[0] = (void*)(i + 1);
        freeList_[i]->next_.vals[1] = freeList_[i + 1];
    }

    freeList_[i]->next_.val = 0;

    SetDoublePointer(head_, (void*)0, freeList_[0]);
    id_ = i;
}


char* LockFreeBuffer::AllocBuffer()
{
    DoublePointer old_head;

    do
    {
        old_head.val = atomic_read_double(&head_);
        if (IsDoublePointerNull(old_head)) return NULL;

        atomic_longlong tmp = ((BufferNode*)old_head.vals[1])->next_.val;
        if (atomic_cas2(&head_.val, old_head.val, tmp)) break;

    } while (1);

    BufferNode* ret = (BufferNode*)old_head.vals[1];

    ret->next_.val = 0;

    return ret->buffer_;
}

void LockFreeBuffer::ReleaseBuffer(void* buffer)
{
    BufferNode* node = container_of(buffer, BufferNode, buffer_);

    assert(node >= freeList_[0] && node <= freeList_[size_ - 1]);

    DoublePointer old_head, new_node;

    size_t id = atomic_increment(&id_);
    SetDoublePointer(new_node, (void*)id, node);

    do
    {
        old_head.val = atomic_read_double(&head_);
        node->next_ = old_head;

        if (atomic_cas2(&head_.val, old_head.val, new_node.val)) break;

    } while (1);
}


