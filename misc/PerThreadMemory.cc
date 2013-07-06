#include "PerThreadMemory.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>

struct Node
{
    Node*  next;
    char   padding[sizeof(void*)];
    char   data[0];
};

//TODO more advance padding.
inline void FillPadding(void* buf, int sz)
{
    memset(buf, 0xcd, sz);
}

inline bool IsPaddingCorrupt(const char* buf, int sz)
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

void PerThreadMemoryAlloc::Init()
{
    pthread_key_create(&m_key, &PerThreadMemoryAlloc::Cleaner);
}

void* PerThreadMemoryAlloc::AllocBuffer()
{
    Node* pHead = NULL;
    if ((pHead = (Node*)pthread_getspecific(m_key)) == NULL) pHead = InitPerThreadList();

    return GetFreeListNode(pHead);
}


Node* PerThreadMemoryAlloc::InitPerThreadList()
{
    void* buf = (Node*)malloc((sizeof(Node) + m_granularity) * (m_population + 1));

    if (buf == NULL) return NULL;

    Node* pHead = new Node;

    pHead->next = (Node*)buf;
    
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


void* PerThreadMemoryAlloc::GetFreeListNode(Node* pHead)
{
    if (pHead == NULL || pHead->next == NULL) return NULL;

    Node* node = pHead->next;
    void* buf  = node->data;

    pHead->next = node->next;

    return buf;
}

void PerThreadMemoryAlloc::ReleaseBuffer(void* buf)
{
    Node* node = (Node*)((char*)buf - sizeof(Node));

    if (node == NULL || IsPaddingCorrupt((char*)buf - sizeof(void*), sizeof(void*))) assert(0);
    
    Node* pHead = (Node*)pthread_getspecific(m_key);
    if (pHead == NULL) return;

    node->next = pHead->next;
    pHead->next = node;
}

void PerThreadMemoryAlloc::Cleaner(void* val)
{
    Node* pHead = (Node*)val;

    free((void*)pHead->next);

    delete pHead;
}

