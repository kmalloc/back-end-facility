#include "lock-free-list.h"
#include <assert.h>
#include <stdlib.h>
#include <iostream>

static const DoublePointer DNULL;

struct ListQueue::LockFreeListNode
{
    void* volatile data;
    DoublePointer next;
    friend void PrintList(ListQueue::LockFreeListNode*);

    static void PrintList(ListQueue::LockFreeListNode*);
};

void ListQueue::LockFreeListNode::PrintList(ListQueue::LockFreeListNode* head)
{
    using namespace std;

    int co = 0;

    while (head)
    {
        cout << "addr:" << hex << head << ", data:" << hex << head->data
             << ", id:" << hex << head->next.lo << endl;

        head = (LockFreeListNode*)(head->next.hi);
        ++co;
    }

    cout << "total:" << co << endl;
}

ListQueue::ListQueue(size_t capacity)
    :m_alloc(sizeof(ListQueue::LockFreeListNode), capacity) 
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

ListQueue::~ListQueue()
{
    // Pop out all data, must do that,
    // otherwise per thread memory is never going to be released.
    void* data;
    while (Pop(data));

    ReleaseNode((LockFreeListNode*)(m_in.hi)); 
}

ListQueue::LockFreeListNode* ListQueue::AllocNode()
{
    LockFreeListNode* tmp = (LockFreeListNode*)m_alloc.AllocBuffer();

    // must initialize it, we have constructor to call.
    // maybe I should have a placement constructor for it.
    if (tmp) tmp->next = DNULL;

    return tmp;
}

void ListQueue::ReleaseNode(LockFreeListNode* node)
{
    m_alloc.ReleaseBuffer(node);
}

bool ListQueue::Push(void* data)
{
    LockFreeListNode* node = AllocNode();

    if (node == NULL) return false;
    
    node->data = data;
    node->next = DNULL;

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

        LockFreeListNode* node_tail = (LockFreeListNode*)(m_in.hi);

        in   = atomic_read_double(&m_in);

        next = atomic_read_double(&node_tail->next);

        if (!IsDoublePointerNull(next)) 
        {
            atomic_cas2((&m_in), in, next);
            continue;
        }

        if (atomic_cas2((&(((LockFreeListNode*)(in.hi))->next)), 0, new_node)) break;
    }

    //this line may fail, but doesn't matter, line 77 will fix it up.
    bool ret = atomic_cas2((&m_in), in, new_node);

    return true;
}

bool ListQueue::Pop(void*& data)
{
    DoublePointer out, in, next;

    while (1)
    {

        out  = atomic_read_double(&m_out);
        in   = atomic_read_double(&m_in);

        LockFreeListNode* node_head = (LockFreeListNode*)(out.hi);
        next = atomic_read_double(&node_head->next);
        
        if (IsDoublePointerNull(next)) return false;

        if (IsDoublePointerEqual(in, out)) 
        {
            atomic_cas2((&m_in), in, next);
            continue;
        }

        // next node may have been released, but it is ok to read.
        // but do not write to it.
        data = ((LockFreeListNode*)(next.hi))->data;

        if (atomic_cas2((&m_out), out, next)) break;
    }

    LockFreeListNode* cur = (LockFreeListNode*)(out.hi);
    ReleaseNode(cur);

    return true;
}
bool ListQueue::Pop(void** data)
{
    if (data == NULL) return false;

    return Pop(*data);
}

