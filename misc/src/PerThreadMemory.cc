#include "PerThreadMemory.h"
#include "sys/atomic_ops.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>

static const size_t gs_padding_sz = 8;
static const unsigned char gs_padding_char[2] = { 0x32, 0x23 };

struct Node
{
    Node* volatile next;
    NodeHead* volatile head;
    unsigned char padding[gs_padding_sz];
    char   data[0];
};

/*
 * dummy buffer in NodeHead will be released only when current thread exits, ensuring that
 * when the last buffer is released by other thread, all buffers will be free.
 */
struct NodeHead
{
    Node* volatile next;
    void* dummy;

    void* volatile mem_frame;
    volatile int node_number;

    const int m_population;
    const int m_granularity;

    NodeHead(int population, int granularity):m_population(population), m_granularity(granularity) {}
};

//TODO more advance padding.
static inline void FillPadding(void* buf, int sz)
{
    memset(buf, gs_padding_char[0], sz/2);
    memset(buf + sz/2, gs_padding_char[1], sz/2);
}

static inline bool IsPaddingCorrupt(const unsigned char* buf, int sz)
{
    for (int i = 0; i < sz/2; ++i)
    {
        if (buf[i] != gs_padding_char[0]) return true;
    }

    for (int i = sz/2; i < sz; ++i)
    {
        if (buf[i] != gs_padding_char[1]) return true;
    }

    return false;
}

PerThreadMemoryAlloc::PerThreadMemoryAlloc(int granularity, int population)
    :m_granularity(granularity%sizeof(int)?granularity + sizeof(int) - granularity%sizeof(int):granularity)
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
    const size_t sz = (sizeof(Node) + m_granularity) * (m_population + 1);

    char* buf = (char*)malloc(sz);
    char* end_buf = buf + sz;

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
        FillPadding(cur->padding, gs_padding_sz);
        cur = cur->next;
    }
    
    assert((char*)cur + sizeof(Node) + m_granularity == end_buf);

    cur->next = NULL;
    cur->head = pHead;
    FillPadding(cur->padding, gs_padding_sz);

    pthread_setspecific(m_key, pHead);

    pHead->dummy = GetFreeBufferFromList(pHead);
    return pHead;
}

// note:
// pHead is different from thread to thread.
// for each list, only the owner thread will have permission to dequeue.
void* PerThreadMemoryAlloc::GetFreeBufferFromList(NodeHead* pHead)
{
    assert(pHead);

    if (pHead->next == NULL) return NULL;

    Node* node;
    void* buf;

    do
    {
        node = pHead->next;

        // accessed from current thread only, node should not be NULL.
        assert(node);

        // keep in mind, only owner thread will dequeue the list.
        // this is the foundamental prerequisite that the following line will not suffer from aba problem.
        if (atomic_cas(&pHead->next, node, node->next))
            break;

    } while(1);

    buf = node->data;

    // no might be less than 0
    // thread that is releasing buffer may not update node_number in time.
    int no = atomic_decrement(&pHead->node_number);

    return buf;
}

// put the buffer back to the list.
// note:
// buffer is allowed to share among threads.
void PerThreadMemoryAlloc::DoReleaseBuffer(void* buf)
{
    Node* node = (Node*)((char*)buf - sizeof(Node));

    if (IsPaddingCorrupt((unsigned char*)buf - gs_padding_sz, gs_padding_sz)) assert(0);
    
    // NodeHead* pHead = (NodeHead*)pthread_getspecific(m_key);
    NodeHead* pHead = node->head;

    Node* old_head;

    do
    {
        old_head = pHead->next;

        // make current node points to the old head node.
        // this should be done before cas.
        node->next = old_head;

        // this is an enqueue operation, aba problem is not an issue.
        // and since only owner thread is allowed to dequeue(calling AllocBuffer()),
        // there is no chance to have aba problem.
        if (atomic_cas(&pHead->next, old_head, node))
            break;

    } while(1);

    atomic_increment(&pHead->node_number);
    assert(pHead->node_number <= pHead->m_population + 1);

    if (pHead->node_number == pHead->m_population + 1)
    {
        Cleaner(pHead);
        return;
    }
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
    assert(pHead->node_number == pHead->m_population + 1);
    
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

