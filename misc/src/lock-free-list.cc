#include "lock-free-list.h"
#include "sys/atomic_ops.h"

#include <assert.h>

struct ListNode
{
    ListNode* next;
    void* data;
};

ListQueue::ListQueue()
    :m_no(0), m_alloc(sizeof(ListNode), 1024)
{
    m_in = new ListNode();
    m_out = m_in;
}

ListQueue::~ListQueue()
{
    // TODO pop all nodes.
    // must do that, otherwise per thread memory is never going to be released.
   delete m_in; 
}


ListNode* ListQueue::AllocNode()
{
    return (ListNode*)m_alloc.AllocBuffer();
}

void ListQueue::ReleaseNode(ListNode* node)
{
    m_alloc.ReleaseBuffer(node);
}

bool ListQueue::Push(void* data)
{
    ListNode* node = AllocNode();

    if (node == NULL) return false;
    
    node->data = data;

    ListNode* in, *next;

    while (1)
    {
        in = m_in;
        next = in->next;

        if (next != NULL) 
        {
            atomic_cas(&m_in, in, next);
            continue;
        }

        if (&atomic_cas(&in->next, NULL, node)) break;
    }

    atomic_cas(&m_in, in, node);
    
    return true;
}

bool ListQueue::Pop(void*& data)
{
    ListNode* out, * next;

    while (1)
    {
        out  = m_out;
        in   = m_in;
        next = out->next;
        
        if (next == NULL) return false;

        if (in == out) atomic_cas(&m_in, in, next);

        data = next->data;

        if (atomic_cas(&m_out, out, next)) break;
    }

    // ABA problem still there.
    // TODO: fix it.
    ReleaseNode(out);
    return true;
}


