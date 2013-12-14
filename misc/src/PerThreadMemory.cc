#include "PerThreadMemory.h"

#include "sys/Defs.h"
#include "sys/AtomicOps.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>


// candy to fill the padding.
// this is used to detect memory corruption.
static const size_t gs_padding_sz = sizeof(void*);
static const unsigned char gs_padding_char[2][gs_padding_sz/2] =
{
    {0x32},
    {0x23},
};


// make sure data is aligned to granularity_
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
    char* volatile dummy;

    void* volatile mem_frame_;
    volatile int node_number;

    const int population_;
    const int granularity_;
    const int offset_;

    // thread id
    pthread_t thread_;

    unsigned char padding[gs_padding_sz];

    NodeHead(int population, int granularity, int offset):population_(population), granularity_(granularity), offset_(offset) {}
};

//TODO more advance padding.
static inline void FillPadding(void* buf)
{
    const int sz = gs_padding_sz/2;
    memcpy(buf, gs_padding_char[0], sz);
    memcpy((char*)buf + sz, gs_padding_char[1], sz);
}

static inline bool IsPaddingCorrupt(const unsigned char* buf)
{
    const int sz = gs_padding_sz/2;

    if (memcmp(buf, gs_padding_char[0], sz)) return true;

    if (memcmp(buf + sz, gs_padding_char[1], sz)) return true;

    return false;
}

PerThreadMemoryAlloc::PerThreadMemoryAlloc(int granularity, int population, int align)
    :align_(AlignTo(align, sizeof(void*)))
    ,granularity_(AlignTo(granularity, align_))
    ,offset_(granularity_ - sizeof(Node)%granularity_)
    ,population_(population), key_(0)
{
    Init();
}

PerThreadMemoryAlloc::~PerThreadMemoryAlloc()
{
}

void PerThreadMemoryAlloc::Init()
{
    pthread_key_create((pthread_key_t*)&key_, &PerThreadMemoryAlloc::OnThreadExit);
}

/*
 * thread calling this function will access the list for its own.
 */
void* PerThreadMemoryAlloc::AllocBuffer() const
{
    NodeHead* pHead = NULL;
    if ((pHead = (NodeHead*)pthread_getspecific(key_)) == NULL) pHead = InitPerThreadList();

    // out of memory;
    if (pHead == NULL) return NULL;

    // a little detection.
    assert(!IsPaddingCorrupt(pHead->padding));

    return GetFreeBufferFromList(pHead);
}

PerThreadMemoryAlloc::NodeHead* PerThreadMemoryAlloc::InitPerThreadList() const
{
    const size_t sz = (sizeof(Node) + offset_ + granularity_) * (population_ + 1);

    char* buf = (char*)malloc(sz);
    char* end_buf = buf + sz;

    if (buf == NULL) return NULL;

    NodeHead* pHead;

    try
    {
        pHead = new NodeHead(population_, granularity_, offset_);
    }
    catch (...)
    {
        free(buf);
        return NULL;
    }

    pHead->thread_ = pthread_self();
    pHead->next = (Node*)buf;
    pHead->node_number = population_ + 1;
    pHead->mem_frame_   = buf;
    FillPadding(pHead->padding);

    Node* cur  = (Node*)buf;

    for (int i = 0; i < population_; ++i)
    {
        cur->head = pHead;
        cur->next = (Node*)((char*)cur + sizeof(Node) + offset_ + granularity_);
        FillPadding(cur->padding);
        cur = cur->next;
    }

    assert((char*)cur + sizeof(Node) + offset_ + granularity_ == end_buf);

    cur->next = NULL;
    cur->head = pHead;
    FillPadding(cur->padding);

    pthread_setspecific(key_, pHead);

    pHead->dummy = (char*)GetFreeBufferFromList(pHead);
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
    buf = node->data + offset_;

    return buf;
}

// put the buffer back to the list.
// note:
// buffer is allowed to share among threads.
void PerThreadMemoryAlloc::DoReleaseBuffer(void* buf)
{
    // Node* node = (Node*)((char*)buf - sizeof(Node));
    Node* node = container_of(buf, PerThreadMemoryAlloc::Node, data);

    assert(!IsPaddingCorrupt((unsigned char*)node->padding));
    assert(node->next == NULL);

    // NodeHead* pHead = (NodeHead*)pthread_getspecific(key_);
    NodeHead* pHead = node->head;

    assert(!IsPaddingCorrupt(pHead->padding));

    memset(buf, 0, pHead->granularity_);

    Node* old_head;

    const int sz = atomic_increment(&pHead->node_number);
    assert(sz <= pHead->population_ + 1);

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

    if (sz == pHead->population_ + 1)
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
    // To support "free" operation from other thread,
    // the following check code must be disable. It is not going to work.
    /*
    NodeHead* head = (NodeHead*)pthread_getspecific(key_);
    if (head == NULL) return;

    assert(buf > head->mem_frame_ && buf < head->mem_frame_ + head->granularity_ * head->population_);
    */

    DoReleaseBuffer((char*)buf - offset_);
}

void PerThreadMemoryAlloc::OnThreadExit(void* val)
{
    NodeHead* pHead = (NodeHead*)val;
    DoReleaseBuffer(pHead->dummy - pHead->offset_);
}

void PerThreadMemoryAlloc::Cleaner(NodeHead* val)
{
    NodeHead* pHead = val;
    Node*     cur   = pHead->next;
    int       co    = 0;

    // make sure all buffers are released when cleaning up.
    assert(pHead->node_number == pHead->population_ + 1);

    while (cur)
    {
        cur = cur->next;
        ++co;
    }

    assert(co == pHead->population_ + 1);

    memset(pHead->padding, 0, gs_padding_sz);

    free(pHead->mem_frame_);
    delete pHead;
}

/*
 * free the memory if no buffer is allocated.
 */
bool PerThreadMemoryAlloc::FreeCurThreadMemory()
{
    NodeHead* pHead = (NodeHead*)pthread_getspecific(key_);
    if (pHead == NULL || pHead->node_number < population_) return false;

    DoReleaseBuffer(pHead->dummy - offset_);
    pthread_setspecific(key_, NULL);

    return true;
}

// buffers that are available at the moment.
int PerThreadMemoryAlloc::Size() const
{
    NodeHead* pHead = (NodeHead*)pthread_getspecific(key_);
    if (pHead == NULL) return 0;

    return pHead->node_number;
}

