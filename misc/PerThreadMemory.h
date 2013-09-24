#ifndef __PER_THREAD_MEMORY_H_
#define __PER_THREAD_MEMORY_H_

#include <pthread.h>

#include "misc/NonCopyable.h"


/*
 * Note:
 *  PerThreadMemoryAlloc works like this:
 *
 *  a)Calling AllocBuffer() will allocate buffer from within current thread's tls.
 *  b)buffer allocated can be shared among different threads.
 *    so the buffer may be released by other thread.
 *
 * this is a typical case of multiple producers, single consumer application scenario.
 *
 * usage:
 * a)call AllocBuffer() to get buffer.
 * b)call ReleaseBuffer() to release buffer.
 *   buffer can be accessed accoss multiple thread, it is safe to do that.
 * c)call FreeCurThreadMemory() to release all buffers in current thread.
 *    this may fail unless all the buffers are released by users.
 *
 * memory of current thread will be free only when:
 * a) current thread exits AND all buffers are released.
 * b) the last buffer is released and the owner thread is exited.
 * c) thread that creates the buffer owns the buffer. 
 *    when buffer is released, it will be put into list of the owner.
 * d) there is no garbage collector. users take responsibility to free(ReleaseBuffer) the resource.
 * d) just use PerThreadMemoryAlloc as malloc. 
 */

class PerThreadMemoryAlloc: public noncopyable
{
    public:

        /* 
         * granularity: buffer size is fixed. this parameter specifys the size of each buffer.
         * population: specifys how many buffers should be created, total number of buffer available.
         * align: specifys the minimal buffer size of each buffer, please note this value will be adjusted internally to
         * make the buffer boundary aligned to word size at least
         */

        PerThreadMemoryAlloc(int granularity, int population, int align = sizeof(void*));
        ~PerThreadMemoryAlloc();

        void* AllocBuffer() const;
        void  ReleaseBuffer(void*) const;
        int   Size() const; 
        int   Granularity() const { return m_granularity; }

        bool  FreeCurThreadMemory();

        pthread_key_t GetPerThreadKey() const { return m_key; }

    private:

        struct Node;
        struct NodeHead;

        void Init();
        static void OnThreadExit(void*);
        static void DoReleaseBuffer(void*);
        static void Cleaner(NodeHead*);
        
        NodeHead* InitPerThreadList() const;
        void* GetFreeBufferFromList(NodeHead*) const;        

        const int m_align; // buffer alignment
        const int m_granularity; // sizeof per buffer
        const int m_offset; //
        const int m_population; // total number of buffer
        
        volatile pthread_key_t m_key;
};

#endif

