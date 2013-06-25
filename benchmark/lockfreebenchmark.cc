#include "sys/atomic_ops.h"
#include "misc/LockFreeContainer.h"
#include "thread/ITask.h"
#include "thread/thread.h"

#include <stdio.h>
#include <assert.h>
#include <semaphore.h>

using namespace std;

class LockFreeConsumerTask: public ITask
{
    public:

        LockFreeConsumerTask()
             :m_stop(false)
             ,m_count(0)
             ,m_box(4096)
        {
            sem_init(&m_sem, 0, 0);
        }

        ~LockFreeConsumerTask() {}

        bool PostItem(void* item = (void*)0x233)
        {
            bool ret = m_box.Push(item);

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

            void* item;

            if (!m_box.Pop(&item) || item == NULL) 
            {
                m_stop = true;
                return NULL;
            }

            return item;
        }

        virtual void Run()
        {
            while (!m_stop)
            {
                void* item = GetItem();

                if (NULL != item && (size_t)item != 0x233) 
                {
                    fprintf(stdout, "%x\n", item);
                    fflush(stdout);
                    assert(0);
                }

                printf("c(%d)\n", m_count);
                atomic_increment(&m_count);
            }

            fprintf(stdout, "c s \n");
            fflush(stdout);
        }

    private:

        volatile bool        m_stop;
        volatile int         m_count;
        sem_t                m_sem;
        LockFreeStack<void*> m_box;

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
            while (!m_stop)
            {
                if (*m_count >= m_max) break;

                printf("p(%d)\n", *m_count);

                if (!m_consumer->PostItem()) 
                {
                    printf("full:%d\n", *m_count);
                    continue;
                }

                atomic_increment(m_count);
            }

            m_consumer->PostItem(NULL);
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
    const int maxSz = 4096;//1 milion
    const int consumerSz = 2, producerSz = 4;

    LockFreeConsumerTask consumer;
    LockFreeProducerTask prod(&consumer, &counter, maxSz - producerSz - 1);

    Thread* consumers[consumerSz];
    Thread* producers[producerSz];

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

    return 0;
}

