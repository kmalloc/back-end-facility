#ifndef __PER_THREAD_MEMORY_H_
#define __PER_THREAD_MEMORY_H_

#include <pthread.h>

class NodeHead;

/*
 * due to short cut design, please be aware:
 *a) all buffers will be free on thread exit, 
     so please be sure to release buffer you allocated from this class.
  b) Call FreeCurThreadMemory() to release all Per thread memory owned 
     by ucrrent thread if you want to release some resource.
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

        static void Cleaner(void*);

        pthread_key_t GetPerThreadKey() const { return m_key; }

    private:

        void Init();
        
        NodeHead* InitPerThreadList();
        void* GetFreeBufferFromList(NodeHead*);        

        const int m_granularity;
        const int m_population;

        pthread_key_t m_key;
};

#endif

