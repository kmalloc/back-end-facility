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

template <class T>
class LockQueue:public SpinlockQueue<T>
{
    public:
        LockQueue(int sz = 2048): SpinlockQueue<T>(sz) {}

        bool Push(const T& val) { return SpinlockQueue<T>::PushBack(val); }
        bool Pop(T* val) { return SpinlockQueue<T>::PopFront(val); }
};


template <class CON>
class LockFreeConsumerTask: public ITask
{
    public:

        LockFreeConsumerTask()
             :m_stop(false)
             ,m_count(0)
             ,m_queue(4096)
        {
            sem_init(&m_sem, 0, 0);
        }

        ~LockFreeConsumerTask() {}

        void StopRun()
        {
            if (!m_stop) m_stop = true;

            PostItem(NULL);
        }

        bool PostItem(void* item = (void*)0x233)
        {
            bool ret = m_queue.Push(item);

            //if (ret) assert(0 == sem_post(&m_sem));

            return ret;
        }

        void* GetItem()
        {
            //sem_wait(&m_sem);

            void* item = NULL;

            bool ret = m_queue.Pop(&item);
            
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
                    printf("emp\n");
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
                    fprintf(stdout, "err,c:%d,v:%x\n", m_count, item);
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

        volatile bool        m_stop;
        volatile int         m_count;
        sem_t                m_sem;
        CON                  m_queue;

};

template<class CON>
class LockFreeProducerTask: public ITask
{
    public:

        LockFreeProducerTask(LockFreeConsumerTask<CON>* consumer, volatile int* counter, volatile int max)
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
                    printf("fu:%d\n", *m_count);
                    sched_yield();
                    continue;
                }

                k = (k + 1)%5;
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
        LockFreeConsumerTask<CON>* m_consumer;
};

int main()
{
    volatile int counter = 0;
    const int maxSz = 1000000; //1 milion
    const int consumerSz = 2, producerSz = 4;

    
    LockFreeConsumerTask<LockQueue<void*> > consumer_lock;
    LockFreeConsumerTask<LockFreeStack<void*> > consumer_lf_stack;
    LockFreeConsumerTask<LockFreeQueue<void*> > consumer_lf_queue;

    LockFreeProducerTask<LockQueue<void*> > prod_lock(&consumer_lock, &counter, maxSz - producerSz - 1);
    LockFreeProducerTask<LockFreeQueue<void*> > prod_lf_queue(&consumer_lf_queue, &counter, maxSz - producerSz - 1);
    LockFreeProducerTask<LockFreeStack<void*> > prod_lf_stack(&consumer_lf_stack, &counter, maxSz - producerSz - 1);

    Thread* consumers[consumerSz];
    Thread* producers[producerSz];

    int input;
    cout << "select container:" << endl;
    cout << "1. spin lock queue" << endl;
    cout << "2. lock free queue" << endl;
    cout << "3. lock free stack" << endl;

    cout << "please input your choice:(1/2/3)" << endl;
    cin >> input; 


    ITask* consumer, *prod;  

    switch (input)
    {
        case 1:
            consumer = & consumer_lock;
            prod     = & prod_lock;
            break;
        case 2:
            consumer = &consumer_lf_queue;
            prod     = &prod_lf_queue;
            break;
        case 3:
            consumer = &consumer_lf_stack;
            prod     = &prod_lf_stack;
            break;
        default:
            assert(0);
    }

    clock_t s, e;

    s = clock();

    for (int i = 0; i < consumerSz; ++i)
    {
        consumers[i] = new Thread(consumer);
        consumers[i]->Start();
    }

    for (int i = 0; i < producerSz; ++i)
    {
        producers[i] = new Thread(prod);
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

