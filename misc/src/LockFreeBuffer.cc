#include "LockFreeBuffer.h"

#include "sys/Defs.h"

#include <assert.h>

struct BufferNode
{
    DoublePointer m_next;
    char m_buffer[0];
};

LockFreeBuffer::LockFreeBuffer(size_t size, size_t granularity)
    :m_size(size), m_granularity(AlignTo(granularity, sizeof(DoublePointer)))
    ,m_id(0), m_rawBuff(NULL), m_freeList(NULL)
{
    size_t sz = m_size * (sizeof(BufferNode) + m_granularity);

    try
    {
        m_rawBuff = new char[sz];
        m_freeList = new BufferNode*[m_size];
    }
    catch (...)
    {
        if (m_rawBuff) delete[] m_rawBuff;
        if (m_freeList) delete[] m_freeList;

        throw "out of memory";
    }

    InitList();
}

LockFreeBuffer::~LockFreeBuffer()
{
    delete[] m_freeList;
    delete[] m_rawBuff;
}

void LockFreeBuffer::InitList()
{
    size_t i, off_set = 0;
    const size_t sz = sizeof(BufferNode) + m_granularity;

    for (i = 0; i < m_size; ++i)
    {
        off_set = i * sz;
        m_freeList[i] = (BufferNode*)(m_rawBuff + off_set);
    }

    for (i = 0; i < m_size - 1; ++i)
    {
        m_freeList[i]->m_next.vals[0] = (void*)(i + 1);
        m_freeList[i]->m_next.vals[1] = m_freeList[i + 1];
    }

    m_freeList[i]->m_next.val = 0;

    SetDoublePointer(m_head, (void*)0, m_freeList[0]);
    m_id = i;
}


char* LockFreeBuffer::AllocBuffer()
{
    DoublePointer old_head;

    do
    {
        old_head.val = atomic_read_double(&m_head);
        if (IsDoublePointerNull(old_head)) return NULL;

        atomic_longlong tmp = ((BufferNode*)old_head.vals[1])->m_next.val;
        if (atomic_cas2(&m_head.val, old_head.val, tmp)) break;

    } while (1);
    
    BufferNode* ret = (BufferNode*)old_head.vals[1];

    ret->m_next.val = 0;

    return ret->m_buffer;
}

void LockFreeBuffer::ReleaseBuffer(void* buffer)
{
    BufferNode* node = container_of(buffer, BufferNode, m_buffer);

    assert(node >= m_freeList[0] && node <= m_freeList[m_size - 1]);

    DoublePointer old_head, new_node;

    size_t id = atomic_increment(&m_id);
    SetDoublePointer(new_node, (void*)id, node);

    do
    {
        old_head.val = atomic_read_double(&m_head);
        node->m_next = old_head;

        if (atomic_cas2(&m_head.val, old_head.val, new_node.val)) break;

    } while (1);
}

