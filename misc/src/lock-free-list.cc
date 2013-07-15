#include "lock-free-list.h"
#include <assert.h>
#include <stdlib.h>

static const DoublePointer DNULL;

struct ListNode
{
    void* data;
    DoublePointer next;
};

ListQueue::ListQueue(size_t capacity)
    :m_alloc(sizeof(ListNode), 1024)
    ,m_id(0)
    ,m_no(0)
    ,m_max(capacity)
{
    ListNode* tmp = AllocNode();
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

    ReleaseNode((ListNode*)(m_in.hi)); 
}

ListNode* ListQueue::AllocNode()
{
    ListNode* tmp = (ListNode*)m_alloc.AllocBuffer();

    //must initialize it, we have constructor to call.
    //maybe I should have a placement constructor for it.
    if (tmp) tmp->next = DNULL;

    return tmp;
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
    node->next = DNULL;

    DoublePointer in, next;
    DoublePointer new_node;

    size_t id = atomic_increment(&m_id);
    SetDoublePointer(new_node, (void*)id, node);

    assert(new_node.lo);

    while (1)
    {
        DoublePointer NullVal;
        in = m_in;
        next = ((ListNode*)(in.hi))->next;

        if (!IsDoublePointerNull(next)) 
        {
            atomic_cas2((&m_in), in, next);
            assert(m_in.lo);
            continue;
        }

        if (atomic_cas2((&(((ListNode*)(in.hi))->next)), NullVal, new_node)) break;
    }

    //this line may fail, but doesn't matter, line 72 will fix it up.
    atomic_cas2((&m_in), in, new_node);
    assert(m_in.lo);
    return true;
}

bool ListQueue::Pop(void*& data)
{
    DoublePointer out, in, next;

    while (1)
    {
        out  = m_out;
        in   = m_in;
        next = ((ListNode*)(out.hi))->next;
        
        if (IsDoublePointerNull(next)) return false;

        if (IsDoublePointerEqual(in, out)) 
        {
            atomic_cas2((&m_in), in, next);
            assert(m_in.lo);
            continue;
        }

        // next node may have been released, but it is ok to read.
        // but do not write to it.
        data = ((ListNode*)(next.hi))->data;

        if (atomic_cas2((&m_out), out, next)) break;
    }

    ReleaseNode((ListNode*)(out.hi));
    return true;
}
bool ListQueue::Pop(void** data)
{
    if (data == NULL) return false;

    return Pop(*data);
}

