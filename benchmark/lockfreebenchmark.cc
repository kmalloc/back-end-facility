#include "sys/atomic_ops.h"
#include "misc/SpinlockQueue.h"
#include "misc/LockFreeContainer.h"
#include "thread/ITask.h"
#include "thread/thread.h"

#include <time.h>
#include <stdio.h>
#include <assert.h>
#include <semaphore.h>

#include <iostream>
using namespace std;

static void* gs_item = (void*)0x233;

class LockFreeConsumerTask: public ITask
{
    public:

        LockFreeConsumerTask(bool lock = false)
             :m_lock(lock)
             ,m_stop(false)
             ,m_count(0)
             ,m_box(4096)
             ,m_lock_queue(4096)
        {
            sem_init(&m_sem, 0, 0);
        }

        ~LockFreeConsumerTask() {}

        bool PostItem(void* item = (void*)0x233)
        {
            bool ret = m_lock?m_lock_queue.PushBack(item):m_box.Push(item);

            if (ret) sem_post(&m_sem);

            return ret;
        }

        void StopRun()
        {
            if (!m_stop) m_stop = true;

            PostItem(NULL);
        }

        void* GetItem()
        {
            sem_wait(&m_sem);

            void* item = NULL;

            bool ret = (!m_lock)?m_box.Pop(&item):m_lock_queue.PopFront(&item);
            
            if (!ret) return NULL;

            return item;
        }

        virtual void Run()
        {
            while (!m_stop)
            {
                void* item = GetItem();

                if (item == NULL)
                {
                    printf("empty\n");
                    sched_yield();
                    continue;
                }
                else if (item == (void*)0x2233)
                    break;

                if (item != (void*)0x233 
                   && item != (void*)(0x233 + 1)
                   && item != (void*)(0x233 + 2)
                   && item != (void*)(0x233 + 3)
                   && item != (void*)(0x233 + 4)) 
                {
                    fprintf(stdout, "err,%x\n", item);
                    fflush(stdout);
                    assert(0);
                }

                //printf("c(%d)\n", m_count);
                atomic_increment(&m_count);
            }

            fprintf(stdout, "q c \n");
            fflush(stdout);
        }

    private:

        const bool           m_lock;
        volatile bool        m_stop;
        volatile int         m_count;
        sem_t                m_sem;
        LockFreeQueue<void*> m_box;
        SpinlockQueue<void*> m_lock_queue;

};


class LockFreeProducerTask: public ITask
{
    public:

        LockFreeProducerTask(LockFreeConsumerTask* consumer, volatile int* counter, volatile int max)
            :m_stop(false), m_count(counter), m_max(max), m_consumer(consumer) 
        {
        }

        ~LockFreeProducerTask() {}

        void Stop()
        {
            m_stop = true;
        }


        virtual void Run()
        {
            int k = 0;
            while (!m_stop)
            {
                if (*m_count >= m_max) break;

                if (!m_consumer->PostItem((void*)(k+0x233))) 
                {
                    printf("full:%d\n", *m_count);
                    sched_yield();
                    continue;
                }

                k = (k+1)%5;
                atomic_increment(m_count);
            }

            while (!m_consumer->PostItem((void*)0x2233))
                sched_yield();

            printf("q p\n");
        }

    private:

        volatile bool       m_stop;
        volatile int*       m_count;
        volatile int        m_max;
        LockFreeConsumerTask* m_consumer;
};

int main()
{
    volatile int counter = 0;
    const int maxSz = 1000000000; //1 milion
    const int consumerSz = 2, producerSz = 4;

    //string input;
    //cout << "lock queue?(y/n):";
    //cin >> input; 
    //bool lock = (input == "y");

    LockFreeConsumerTask consumer(false);
    LockFreeProducerTask prod(&consumer, &counter, maxSz - producerSz - 1);

    Thread* consumers[consumerSz];
    Thread* producers[producerSz];

    clock_t s, e;

    s = clock();

    for (int i = 0; i < consumerSz; ++i)
    {
        consumers[i] = new Thread(&consumer);
        consumers[i]->Start();
    }

    for (int i = 0; i < producerSz; ++i)
    {
        producers[i] = new Thread(&prod);
        producers[i]->Start();
    }

    for (int i = 0; i < producerSz; ++i)
    {
        producers[i]->Join();
    }

    for (int i = 0; i < consumerSz; ++i)
    {
        consumers[i]->Join();
    }

    e = clock();

    float total = (double)(e - s)/(double)CLOCKS_PER_SEC;

    printf("items:%d, max:%d\n", counter, maxSz);
    printf("total time :%f(s)\n", total);
    return 0;
}

