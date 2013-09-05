#include "LockFreeBuffer.h"

struct BufferNode
{
    DoublePointer m_next;
    char* m_buffer[0];
};


LockFreeBuffer::LockFreeBuffer(size_t size, size_t granularity)
    :m_size(size), m_granularity(AlignTo(granularity, sizeof(DoublePointer)))
    ,m_freeList(NULL), m_id(0)
{
    size_t sz = m_size * (sizeof(BufferNode) + m_granularity);

    m_rawBuff = new char[sz];

    InitList();
}

LockFreeBuffer::~LockFreeBuffer()
{
    delete[] m_rawBuff;
}

void LockFreeBuffer::InitList()
{
    assert(!m_freeList);

    m_freeList = (BufferNode*)m_rawBuff;
    
    size_t i = 0;
    for (; i < m_size - 1; ++i)
    {
        m_freeList[i].m_next.vals[0] = (void*)(i + 1);
        m_freeList[i].m_next.vals[1] = &m_freeList[i + 1];
    }

    m_freeList[i].next = 0;

    SetDoublePointer(m_head, (void*)0, m_freeList);
    m_id = i;
}


char* LockFreeBuffer::AllocBuffer()
{
    // TODO
    //
    return NULL;
}

void LockFreeBuffer::ReleaseBuffer()
{
    // TODO
}


