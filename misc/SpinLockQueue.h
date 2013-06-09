#ifndef _SPIN_LOCK_QUEUE_H_
#define _SPIN_LOCK_QUEUE_H_

#include "defs.h"

#include <queue>
#include <pthread.h>
#include <semaphore.h>

template<class Type>
class SpinLockQueue
{
    public:

        SpinLockQueue(int Maxsize = DEFAULT_WORKER_TASK_MSG_SIZE)
            :m_maxSz(Maxsize)
        {
            pthread_spin_init(&m_lock,0);
        }

        ~SpinLockQueue()
        {
        }

        Type PopFront()
        {
            Type val;

            pthread_spin_lock(&m_lock);

            if (m_queue.empty())
            {
                pthread_spin_unlock(&m_lock);
                throw "queue empty";
            }

            val = m_queue.front();
            m_queue.pop();
            pthread_spin_unlock(&m_lock);

            return val;
        }

        bool PushBack(Type val)
        {
            bool full = true;

            pthread_spin_lock(&m_lock);
            full = m_queue.size() > m_maxSz;

            if (!full)
            {
                m_queue.push(val);
            }

            pthread_spin_unlock(&m_lock);

            return !full;
        }

        void Clear()
        {
            pthread_spin_lock(&m_lock);
            m_queue.clear();
            pthread_spin_unlock(&m_lock);
        }

        int  Size() 
        {
            int count = -1;

            pthread_spin_lock(&m_lock);
            count = m_queue.size();
            pthread_spin_unlock(&m_lock);

            return count;
        }

        bool IsEmpty() 
        {
            bool ret;

            pthread_spin_lock(&m_lock);
            ret = m_queue.empty();
            pthread_spin_unlock(&m_lock);

            return ret;
        }

    private:

        const int m_maxSz;
        volatile pthread_spinlock_t m_lock;
        std::queue <Type> m_queue;
};


#endif

