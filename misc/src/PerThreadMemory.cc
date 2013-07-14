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

/*
 * dummy buffer in NodeHead will be released only when current thread exits, ensuring that
 * when the last buffer is released by other thread, all buffers will be free.
 */
struct NodeHead
{
    Node* next;
    void* dummy;

    void* mem_frame;
    int   node_number;

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
 * thread calling this function will access the list for its own.
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
    pHead->node_number = m_population + 1;
    pHead->mem_frame   = buf;
    
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

// note:
// pHead is different from thread to thread.
// for each list, only the owner thread will have permission to dequeue.
void* PerThreadMemoryAlloc::GetFreeBufferFromList(NodeHead* pHead)
{
    if (pHead == NULL || pHead->next == NULL) return NULL;

    Node* node;
    void* buf;

    do
    {
        node = pHead->next;

        //keep in mind, only owner thread will dequeue the list.
        //this is the foundamental prerequisite that the following line will not suffer from aba problem.
        if (atomic_cas(&pHead->next, node, node->next))
            break;

    } while(1);

    buf = node->data;
    atomic_decrement(&pHead->node_number);

    assert(pHead->node_number >= 0);
    return buf;
}

// put the buffer back to the list.
// note:
// buffer is allowed to share among threads.
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
        old_size = pHead->node_number;

        //make current node points to the old head node.
        //this should be done before cas.
        node->next = old_head;

        // this is an enqueue operation, aba problem is not an issue.
        // and since only owner thread is allowed to dequeue(calling AllocBuffer()),
        // there is no chance to have aba problem.
        if (atomic_cas(&pHead->next, old_head, node))
            break;

    } while(1);

    if (old_size == population)
    {
        Cleaner(pHead);
        return;
    }

    atomic_increment(&pHead->node_number);
}

/*
 *this function will be called across different threads.
 *so need to apply extra care to make it work as expected.
 */
void PerThreadMemoryAlloc::ReleaseBuffer(void* buf)
{
    // support free operation from other thread.
    // the following check code is not going work.
    /*
    NodeHead* head = (NodeHead*)pthread_getspecific(m_key);
    if (head == NULL) return;

    assert(buf > head->mem_frame && buf < head->mem_frame + head->m_granularity * head->m_population);
    */

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

    // make sure all buffers are released when cleaning up.
    assert(pHead->node_number == pHead->m_population);
    
    free(pHead->mem_frame);

    delete pHead;
}

/*
 * free the memory if no buffer is allocated.
 */
bool PerThreadMemoryAlloc::FreeCurThreadMemory()
{
    NodeHead* pHead = (NodeHead*)pthread_getspecific(m_key);
    if (pHead == NULL || pHead->node_number < m_population) return false;

    DoReleaseBuffer(pHead->dummy);
    pthread_setspecific(m_key, NULL);

    return true;
}

// buffers that are available at the moment.
int PerThreadMemoryAlloc::Size() const
{
    NodeHead* pHead = (NodeHead*)pthread_getspecific(m_key);
    if (pHead == NULL) return 0;

    return pHead->node_number;
}

