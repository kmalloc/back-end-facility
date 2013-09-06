#include "PerThreadMemory.h"

#include "sys/defs.h"
#include "sys/atomic_ops.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>



static const size_t gs_padding_sz = sizeof(void*);
static const unsigned char gs_padding_char[2] = { 0x32, 0x23 };

// make sure data is aligned to m_granularity
// some application relys on that.
struct PerThreadMemoryAlloc::Node
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
struct PerThreadMemoryAlloc::NodeHead
{
    Node* volatile next;
    void* volatile dummy;

    void* volatile mem_frame;
    volatile int node_number;

    const int m_offset;
    const int m_population;
    const int m_granularity;

    // thread id
    pthread_t m_thread;

    unsigned char padding[gs_padding_sz];

    NodeHead(int population, int granularity, int offset):m_population(population), m_granularity(granularity), m_offset(offset) {}
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

PerThreadMemoryAlloc::PerThreadMemoryAlloc(int granularity, int population, int align)
    :m_align(AlignTo(align, sizeof(void*)))
    ,m_granularity(AlignTo(granularity, m_align))
    ,m_offset(m_granularity - sizeof(Node)%m_granularity)
    ,m_population(population), m_key(0)
{
    Init();
}

PerThreadMemoryAlloc::~PerThreadMemoryAlloc()
{
}

void PerThreadMemoryAlloc::Init()
{
    pthread_key_create((pthread_key_t*)&m_key, &PerThreadMemoryAlloc::OnThreadExit);
}

/*
 * thread calling this function will access the list for its own.
 */
void* PerThreadMemoryAlloc::AllocBuffer() const
{
    NodeHead* pHead = NULL;
    if ((pHead = (NodeHead*)pthread_getspecific(m_key)) == NULL) pHead = InitPerThreadList();

    // a little detection.
    assert(!IsPaddingCorrupt(pHead->padding, gs_padding_sz));

    return GetFreeBufferFromList(pHead);
}

PerThreadMemoryAlloc::NodeHead* PerThreadMemoryAlloc::InitPerThreadList() const
{
    const size_t sz = (sizeof(Node) + m_offset + m_granularity) * (m_population + 1);

    char* buf = (char*)malloc(sz);
    char* end_buf = buf + sz;

    if (buf == NULL) return NULL;

    NodeHead* pHead = new NodeHead(m_population, m_granularity, m_offset);

    pHead->m_thread = pthread_self();
    pHead->next = (Node*)buf;
    pHead->node_number = m_population + 1;
    pHead->mem_frame   = buf;
    FillPadding(pHead->padding, gs_padding_sz);
    
    Node* cur  = (Node*)buf;

    for (int i = 0; i < m_population; ++i)
    {
        cur->head = pHead;
        cur->next = (Node*)((char*)cur + sizeof(Node) + m_offset + m_granularity);
        FillPadding(cur->padding, gs_padding_sz);
        cur = cur->next;
    }
    
    assert((char*)cur + sizeof(Node) + m_offset + m_granularity == end_buf);

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
void* PerThreadMemoryAlloc::GetFreeBufferFromList(NodeHead* pHead) const
{
    if (pHead->next == NULL) return NULL;

    Node* node;
    void* buf;

    int no = atomic_decrement(&pHead->node_number);

    assert(no);

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

    node->next = NULL;
    buf = node->data + m_offset;

    return buf;
}

// put the buffer back to the list.
// note:
// buffer is allowed to share among threads.
void PerThreadMemoryAlloc::DoReleaseBuffer(void* buf)
{
    //Node* node = (Node*)((char*)buf - sizeof(Node));
    Node* node = container_of(buf, PerThreadMemoryAlloc::Node, data); 

    assert(!IsPaddingCorrupt((unsigned char*)node->padding, gs_padding_sz));
    assert(node->next == NULL);

    // NodeHead* pHead = (NodeHead*)pthread_getspecific(m_key);
    NodeHead* pHead = node->head;

    assert(!IsPaddingCorrupt(pHead->padding, gs_padding_sz));

    memset(buf, 0, pHead->m_granularity);

    Node* old_head;

    const int sz = atomic_increment(&pHead->node_number);
    assert(sz <= pHead->m_population + 1);

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

    if (sz == pHead->m_population + 1)
    {
        Cleaner(pHead);
        return;
    }
}

/*
 *this function will be called across different threads.
 *so need to apply extra care to make it work as expected.
 */
void PerThreadMemoryAlloc::ReleaseBuffer(void* buf) const
{
    // To support free operation from other thread,
    // the following check code must be disable. It is not going to work.
    /*
    NodeHead* head = (NodeHead*)pthread_getspecific(m_key);
    if (head == NULL) return;

    assert(buf > head->mem_frame && buf < head->mem_frame + head->m_granularity * head->m_population);
    */

    DoReleaseBuffer(buf - m_offset);
}

void PerThreadMemoryAlloc::OnThreadExit(void* val)
{
    NodeHead* pHead = (NodeHead*)val;
    DoReleaseBuffer(pHead->dummy - pHead->m_offset);
}

void PerThreadMemoryAlloc::Cleaner(NodeHead* val)
{
    NodeHead* pHead = val;
    Node*     cur   = pHead->next;
    int       co    = 0;

    // make sure all buffers are released when cleaning up.
    assert(pHead->node_number == pHead->m_population + 1);
    
    while (cur) 
    {
        cur = cur->next;
        ++co;
    }

    assert(co == pHead->m_population + 1);

    memset(pHead->padding, 0, gs_padding_sz);

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

    DoReleaseBuffer(pHead->dummy - m_offset);
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

