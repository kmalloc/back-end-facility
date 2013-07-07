#include "PerThreadMemory.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>

struct Node
{
    Node*  next;
    unsigned char padding[sizeof(void*)];
    char   data[0];
};

struct NodeHead
{
    Node* next;
    void* buf;
    int   size;

    const int m_population;
    const int m_granularity;

    NodeHead(int population, int granularity):m_population(population), m_granularity(granularity) {}
};

//TODO more advance padding.
inline void FillPadding(void* buf, int sz)
{
    memset(buf, 0xcd, sz);
}

inline bool IsPaddingCorrupt(const unsigned char* buf, int sz)
{
    for (int i = 0; i < sz; ++i)
    {
        if (buf[i] != 0xcd) return true;
    }

    return false;
}

PerThreadMemoryAlloc::PerThreadMemoryAlloc(int granularity, int population)
    :m_granularity(granularity + sizeof(int) - granularity%sizeof(int)), m_population(population), m_key(0)
{
    Init();
}

PerThreadMemoryAlloc::~PerThreadMemoryAlloc()
{
}

void PerThreadMemoryAlloc::Init()
{
    pthread_key_create(&m_key, &PerThreadMemoryAlloc::Cleaner);
}

void* PerThreadMemoryAlloc::AllocBuffer()
{
    NodeHead* pHead = NULL;
    if ((pHead = (NodeHead*)pthread_getspecific(m_key)) == NULL) pHead = InitPerThreadList();

    return GetFreeBufferFromList(pHead);
}

NodeHead* PerThreadMemoryAlloc::InitPerThreadList()
{
    void* buf = (Node*)malloc((sizeof(Node) + m_granularity) * (m_population + 1));

    if (buf == NULL) return NULL;

    NodeHead* pHead = new NodeHead(m_population, m_granularity);

    pHead->next = (Node*)buf;
    pHead->size = m_population;
    pHead->buf  = buf;
    
    Node* cur  = (Node*)buf;

    for (int i = 0; i < m_population - 1; ++i)
    {
        cur->next = (Node*)((char*)cur + sizeof(Node) + m_granularity);
        FillPadding(cur->padding, sizeof(void*));
        cur = cur->next;
    }

    cur->next = NULL;
    FillPadding(cur->padding, sizeof(void*));

    pthread_setspecific(m_key, pHead);
    return pHead;
}

void* PerThreadMemoryAlloc::GetFreeBufferFromList(NodeHead* pHead)
{
    if (pHead == NULL || pHead->next == NULL) return NULL;

    Node* node = pHead->next;
    void* buf  = node->data;

    pHead->next = node->next;
    pHead->size -= 1;
    return buf;
}

void PerThreadMemoryAlloc::ReleaseBuffer(void* buf)
{
    Node* node = (Node*)((char*)buf - sizeof(Node));

    if (node == NULL || IsPaddingCorrupt((unsigned char*)buf - sizeof(void*), sizeof(void*))) assert(0);
    
    NodeHead* pHead = (NodeHead*)pthread_getspecific(m_key);
    if (pHead == NULL) return;

    node->next = pHead->next;
    pHead->next = node;
    pHead->size += 1;
}

void PerThreadMemoryAlloc::Cleaner(void* val)
{
    NodeHead* pHead = (NodeHead*)val;

    //make sure all buffers are released when cleaning up.
    assert(pHead->size == pHead->m_population);
    
    free(pHead->buf);

    delete pHead;
}

void PerThreadMemoryAlloc::FreeCurThreadMemory()
{
    NodeHead* pHead = (NodeHead*)pthread_getspecific(m_key);
    if (pHead == NULL) return;

    Cleaner(pHead);

    pthread_setspecific(m_key, NULL);
}

int PerThreadMemoryAlloc::Size() const
{
    NodeHead* pHead = (NodeHead*)pthread_getspecific(m_key);
    if (pHead == NULL) return 0;

    return pHead->size;
}

