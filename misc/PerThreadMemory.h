#ifndef __PER_THREAD_MEMORY_H_
#define __PER_THREAD_MEMORY_H_

#include <pthread.h>

class NodeHead;

/*
 * usage:
 * a)call AllocBuffer() to get buffer.
 * b)call ReleaseBuffer() to release buffer.
 * buffer can be accessed accoss multiple thread, it is safe to do that.
 *
 * buffers will be free only when:
 * a) thread exits AND all buffers are released.
 * b) last buffer released and the owner thread is exited.
 * c) thread that creates the buffer owns the buffer. 
 *    when buffer is released, it will be put into list of the owner.
 *
 * dummy buffer in NodeHead will be release only when current thread exits, ensuring that
 * when the last buffer is released by other thread, all buffers will be free.
 */

class PerThreadMemoryAlloc
{
    public:

        PerThreadMemoryAlloc(int granularity, int population);
        ~PerThreadMemoryAlloc();

        void* AllocBuffer();
        void  ReleaseBuffer(void*);
        int   Size() const; 
        int   Granularity() const { return m_granularity; }
        void  FreeCurThreadMemory();

        static void OnThreadExit(void*);

        pthread_key_t GetPerThreadKey() const { return m_key; }

    private:

        void Init();
        static void DoReleaseBuffer(void*);
        static void Cleaner(NodeHead*);
        
        NodeHead* InitPerThreadList();
        void* GetFreeBufferFromList(NodeHead*);        

        const int m_granularity;
        const int m_population;

        pthread_key_t m_key;
};

#endif

