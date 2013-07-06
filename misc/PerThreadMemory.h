#ifndef __PER_THREAD_MEMORY_H_
#define __PER_THREAD_MEMORY_H_

#include <pthread.h>

class Node;

class PerThreadMemoryAlloc
{
    public:

        PerThreadMemoryAlloc(int granularity, int population);

        void* AllocBuffer();
        void ReleaseBuffer(void*);

        static void Cleaner(void*);

    private:

        ~PerThreadMemoryAlloc();
        void Init();
        
        Node* InitPerThreadList();
        void* GetFreeListNode(Node*);        

        const int m_granularity;
        const int m_population;

        pthread_key_t m_key;
};

#endif

