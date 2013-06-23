#include "atomic_ops.h"
#include "LockFreeContainer.h"
#include "thread.h"

#include <assert.h>
#include <semaphore.h>

class LockFreeConsumerTask: public ITask
{
    public:

        LockFreeConsumerTask()
            :m_stop(false)
            ,m_count(0)
            ,m_box(2048)
            ,m_default((void*)0x233)
        {
            sem_init(&m_sem, 0, 0);
        }

        ~LockFreeConsumerTask() {}

        bool PostItem(void* item = m_default)
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

            if (!m_box.Pop(&item)) assert(0);

            return item;
        }

        virtual void Run()
        {
            while (!m_stop)
            {
                void* item = GetItem();

                atomic_increment(&m_count);

                assert((size_t)item == 0x233);
            }
        }

    private:

        volatile bool        m_stop;
        volatile size_t      m_count;
        sem_t                m_sig;
        LockFreeStack<void*> m_box;

        const void*          m_default;
};


class LockFreeProducerTask: public ITask
{
    public:

        LockFreeProducerTask(volatile LockFreeConsumerTask* consumer, volatile size_t* counter, volatile size_t max)
            :m_stop(false), m_count(counter), m_max(max),m_consumer(consumer) 
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
                if (!m_consumer.PostItem()) sleep(1);

                atomic_increment(m_count);

                if (*m_count > m_max) break;
            }
        }

    private:

        volatile bool       m_stop;
        volatile size_t*    m_count;
        volatile size_t*    m_max;
        volatile LockFreeConsumerTask* m_consumer;
};

int main()
{
    return 0;
}

