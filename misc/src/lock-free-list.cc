#include "lock-free-list.h"
#include <assert.h>
#include <stdlib.h>
#include <iostream>


struct LockFreeListQueue::LockFreeListNode
{
    DoublePointer next;
    void* volatile data;

    static void PrintList(LockFreeListQueue::LockFreeListNode*);
};

void LockFreeListQueue::LockFreeListNode::PrintList(LockFreeListQueue::LockFreeListNode* head)
{
    using namespace std;

    int co = 0;

    while (head)
    {
        cout << "addr:" << hex << head << ", data:" << hex << head->data
             << ", id:" << hex << head->next.vals[0] << endl;

        head = (LockFreeListNode*)(head->next.vals[1]);
        ++co;
    }

    cout << "total:" << co << endl;
}

LockFreeListQueue::LockFreeListQueue(size_t capacity)
    :m_alloc(sizeof(LockFreeListQueue::LockFreeListNode), capacity) 
    ,m_id(0)
    ,m_no(0)
    ,m_max(capacity)
{
    LockFreeListNode* tmp = AllocNode();
    assert(tmp);
    SetDoublePointer(m_in, (void*)m_id, tmp);
    m_id = 1;
    m_out = m_in;
}

LockFreeListQueue::~LockFreeListQueue()
{
    // Pop out all data, must do that,
    // otherwise per thread memory is never going to be released.
    void* data;
    while (Pop(data));

    ReleaseNode((LockFreeListNode*)(m_in.vals[1])); 
}

LockFreeListQueue::LockFreeListNode* LockFreeListQueue::AllocNode()
{
    LockFreeListNode* tmp = (LockFreeListNode*)m_alloc.AllocBuffer();

    // must initialize node manually, we don't have constructor to call.
    // maybe I should have provided a placement constructor for it.
    if (tmp) tmp->next = 0;

    return tmp;
}

void LockFreeListQueue::ReleaseNode(LockFreeListNode* node)
{
    m_alloc.ReleaseBuffer(node);
}

bool LockFreeListQueue::Push(void* data)
{
    LockFreeListNode* node = AllocNode();

    if (node == NULL) return false;
    
    node->data = data;
    node->next = 0;

    DoublePointer in, next;
    DoublePointer new_node;
    DoublePointer NullVal;

    size_t id = atomic_increment(&m_id);
    SetDoublePointer(new_node, (void*)id, node);

    while (1)
    {
        // just a reminder:
        // DoublePointer1 = DoublePointer2;
        // is not an atomic operation.

        LockFreeListNode* node_tail = (LockFreeListNode*)(m_in.vals[1]);

        in   = atomic_read_double(&m_in);

        next = atomic_read_double(&node_tail->next);

        if (!IsDoublePointerNull(next)) 
        {
            atomic_cas2(&(m_in.val), in.val, next.val);
            continue;
        }

        if (atomic_cas2((&(((LockFreeListNode*)(in.vals[1]))->next.val)), 0, new_node.val)) break;
    }

    // this line may fail, but doesn't matter, line 104 will fix it up.
    bool ret = atomic_cas2((&m_in.val), in.val, new_node.val);

    return true;
}

bool LockFreeListQueue::Pop(void*& data)
{
    DoublePointer out, in, next;

    while (1)
    {
        out  = atomic_read_double(&m_out);
        in   = atomic_read_double(&m_in);

        LockFreeListNode* node_head = (LockFreeListNode*)(out.vals[1]);
        next = atomic_read_double(&node_head->next);
        
        if (IsDoublePointerNull(next)) return false;

        if (IsDoublePointerEqual(in, out)) 
        {
            atomic_cas2((&m_in.val), in.val, next.val);
            continue;
        }

        // next node may have been released, but it is ok to read.
        // but do not write to it.
        data = ((LockFreeListNode*)(next.vals[1]))->data;

        if (atomic_cas2((&m_out.val), out.val, next.val)) break;
    }

    LockFreeListNode* cur = (LockFreeListNode*)(out.vals[1]);
    ReleaseNode(cur);

    return true;
}
bool LockFreeListQueue::Pop(void** data)
{
    if (data == NULL) return false;

    return Pop(*data);
}

