#include "PerThreadMemory.h"
#include "sys/atomic_ops.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>

struct Node
{
    Node*  next;
    NodeHead* head;
    unsigned char padding[sizeof(void*)];
    char   data[0];
};

struct NodeHead
{
    Node* next;
    void* dummy;

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
    :m_granularity(granularity + sizeof(int) - granularity%sizeof(int))
    ,m_population(population), m_key(0)
{
    Init();
}

PerThreadMemoryAlloc::~PerThreadMemoryAlloc()
{
}

void PerThreadMemoryAlloc::Init()
{
    pthread_key_create(&m_key, &PerThreadMemoryAlloc::OnThreadExit);
}

/*
 *this function will be called from the owner thread only, so it is safe.
 */
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
    pHead->size = m_population + 1;
    pHead->buf  = buf;
    
    Node* cur  = (Node*)buf;

    for (int i = 0; i < m_population; ++i)
    {
        cur->head = pHead;
        cur->next = (Node*)((char*)cur + sizeof(Node) + m_granularity);
        FillPadding(cur->padding, sizeof(void*));
        cur = cur->next;
    }

    cur->next = NULL;
    cur->head = pHead;
    FillPadding(cur->padding, sizeof(void*));

    pthread_setspecific(m_key, pHead);

    pHead->dummy = GetFreeBufferFromList(pHead);
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

void PerThreadMemoryAlloc::DoReleaseBuffer(void* buf)
{
    Node* node = (Node*)((char*)buf - sizeof(Node));

    if (node == NULL || IsPaddingCorrupt((unsigned char*)buf - sizeof(void*), sizeof(void*))) assert(0);
    
    //NodeHead* pHead = (NodeHead*)pthread_getspecific(m_key);
    NodeHead* pHead = node->head;
    if (pHead == NULL) return;

    int   population = pHead->m_population;
    int   old_size;
    Node* old_head;

    do
    {
        old_head = pHead->next;
        old_size = pHead->size;

        // this is an enqueue operation, aba problem is not an issue.
        // and since only owner thread is allowed to dequeue(calling AllocBuffer()),
        // there is no chance to have aba problem.
        if (atomic_cas(&pHead->next, old_head, node))
            break;

    } while(1);

    node->next = old_head;
    if (old_size == population)
    {
        Cleaner(pHead);
        return;
    }

    atomic_increment(&pHead->size);
}

/*
 *this function will be called across different threads.
 *so need to apply extra care to make it work as expected.
 */
void PerThreadMemoryAlloc::ReleaseBuffer(void* buf)
{
    DoReleaseBuffer(buf);
}

void PerThreadMemoryAlloc::OnThreadExit(void* val)
{
    NodeHead* pHead = (NodeHead*)val;
    DoReleaseBuffer(pHead->dummy);
}

void PerThreadMemoryAlloc::Cleaner(NodeHead* val)
{
    NodeHead* pHead = val;

    //make sure all buffers are released when cleaning up.
    assert(pHead->size == pHead->m_population);
    
    free(pHead->buf);

    delete pHead;
}

bool PerThreadMemoryAlloc::FreeCurThreadMemory()
{
    NodeHead* pHead = (NodeHead*)pthread_getspecific(m_key);
    if (pHead == NULL || pHead->size < m_population) return false;

    Cleaner(pHead);

    pthread_setspecific(m_key, NULL);
}

int PerThreadMemoryAlloc::Size() const
{
    NodeHead* pHead = (NodeHead*)pthread_getspecific(m_key);
    if (pHead == NULL) return 0;

    return pHead->size;
}

