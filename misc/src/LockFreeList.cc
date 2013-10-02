#include "LockFreeList.h"
#include <assert.h>
#include <stdlib.h>
#include <iostream>


/*
 * all the nodes will either be linked in the lock-free list(using next) or
 * linked in the unused-list (using next2)
 */
struct LockFreeListQueue::LockFreeListNode
{
    DoublePointer next; // lock free queue pointer
    DoublePointer next2; // internal node list pointer

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
    :m_id(0)
    ,m_no(0)
    ,m_max(capacity)
    ,m_id2(0)
    ,m_freeList(NULL)
{
    InitInternalNodeList();
    LockFreeListNode* tmp = AllocNode();
    assert(tmp);
    SetDoublePointer(m_in, (void*)m_id, tmp);
    m_id = 1;
    m_out = m_in;
}

LockFreeListQueue::~LockFreeListQueue()
{
    delete[] m_freeList;
}

void LockFreeListQueue::InitInternalNodeList()
{
    assert(!m_freeList);

    m_freeList = new LockFreeListNode[m_max];        

    size_t i = 0;
    for (; i < m_max - 1; ++i)
    {
        m_freeList[i].next2.vals[0] = (void*)(i + 1);
        m_freeList[i].next2.vals[1] = &m_freeList[i + 1];
    }

    m_freeList[i].next2.val = 0;

    SetDoublePointer(m_head, (void*)0, m_freeList);
    m_id2 = i;
}

LockFreeListQueue::LockFreeListNode* LockFreeListQueue::AllocNode()
{
    DoublePointer old_head;

    do
    {
        old_head.val = atomic_read_double(&m_head);
        if (IsDoublePointerNull(old_head)) return NULL;

        // m_freeList will never be freed untill queue is destroyed.
        // so it is safe to access old_head->next
        if (atomic_cas2(&m_head.val, old_head.val, ((LockFreeListNode*)old_head.vals[1])->next2.val)) break;

    } while (1);

    LockFreeListNode* ret = (LockFreeListNode*)old_head.vals[1];

    // must initialize node manually, we don't have constructor to call.
    // maybe I should have provided a placement constructor for it.
    ret->next.val = 0;

    return ret;
}

void LockFreeListQueue::ReleaseNode(LockFreeListNode* node)
{
    assert(node >= m_freeList && node < m_freeList + m_max);

    DoublePointer old_head;
    DoublePointer new_node;

    size_t id = atomic_increment(&m_id2);
    SetDoublePointer(new_node, (void*)id, node);

    do
    {
        old_head.val = atomic_read_double(&m_head);
        node->next2 = old_head;

        if (atomic_cas2(&m_head.val, old_head.val, new_node.val)) break;

    } while (1);
}

bool LockFreeListQueue::Push(void* data)
{
    LockFreeListNode* node = AllocNode();

    if (node == NULL) return false;
    
    node->data = data;
    node->next.val = 0;

    DoublePointer in, next;
    DoublePointer new_node;

    size_t id = atomic_increment(&m_id);
    SetDoublePointer(new_node, (void*)id, node);

    while (1)
    {
        // just a reminder:
        // DoublePointer1 = DoublePointer2;
        // is not an atomic operation.

        in.val = atomic_read_double(&m_in);
        LockFreeListNode* node_tail = (LockFreeListNode*)atomic_read(&in.vals[1]);
        next.val = atomic_read_double(&node_tail->next);

        if (!IsDoublePointerNull(next)) 
        {
            atomic_cas2(&(m_in.val), in.val, next.val);
            continue;
        }
        
        if (atomic_cas2((&(((LockFreeListNode*)(in.vals[1]))->next.val)), 0, new_node.val)) break;
    }

    // this line may fail, but doesn't matter, line 149 will fix it up.
    atomic_cas2((&m_in.val), in.val, new_node.val);

    return true;
}

bool LockFreeListQueue::Pop(void*& data)
{
    DoublePointer out, in, next;

    while (1)
    {
        out.val  = atomic_read_double(&m_out);
        in.val   = atomic_read_double(&m_in);

        LockFreeListNode* node_head = (LockFreeListNode*)atomic_read(&out.vals[1]);
        next.val = atomic_read_double(&node_head->next);
        
        if (IsDoublePointerNull(next)) return false;

        if (IsDoublePointerEqual(in, out)) 
        {
            atomic_cas2(&(m_in.val), in.val, next.val);
            continue;
        }

        // LockFreeListNode will  never be freed until queue is destroyed.
        // so it is safe to access data, even when that node is released.
        data = ((LockFreeListNode*)(next.vals[1]))->data;

        if (atomic_cas2(&(m_out.val), out.val, next.val)) break;
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

