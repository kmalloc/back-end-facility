#ifndef _SPIN_LOCK_QUEUE_H_
#define _SPIN_LOCK_QUEUE_H_

#include "defs.h"

#include <queue>
#include <pthread.h>
#include <semaphore.h>

template<class Type>
class SpinlockQueue
{
    public:

        SpinlockQueue(int Maxsize = DEFAULT_WORKER_TASK_MSG_SIZE)
            :m_maxSz(Maxsize)
        {
            pthread_spin_init(&m_lock,0);
        }

        ~SpinlockQueue()
        {
        }

        //set up null value.
        //this value will be return on when pop on empty queue.
        void SetNullValue(const Type& val)
        {
            m_null = val;
        }

        Type PopFront()
        {
            Type val;

            pthread_spin_lock(&m_lock);

            if (m_queue.empty())
            {
                pthread_spin_unlock(&m_lock);
                return m_null;
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
            full = m_queue.size() >= m_maxSz;

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

        Type m_null; //null value, this value will be return if pop on empty queue.
        const int m_maxSz;
        volatile pthread_spinlock_t m_lock;
        std::queue <Type> m_queue;
};


#endif

