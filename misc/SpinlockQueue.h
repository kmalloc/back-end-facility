#ifndef _SPIN_LOCK_QUEUE_H_
#define _SPIN_LOCK_QUEUE_H_

#include "defs.h"

#include <vector>
#include <queue>
#include <pthread.h>
#include <semaphore.h>

template<class Type, class cmp = std::less<Type> >
class PriorityQueue
{
    public:
        PriorityQueue() {}
        ~PriorityQueue(){}

        const Type& front()
        {
            return m_queue.top();
        }

        void pop()
        {
            m_queue.pop();
        }

        void push(const Type& x)
        {
            m_queue.push(x);
        }

        bool empty() const
        {
            return m_queue.empty();
        }

        size_t size() const
        {
            return m_queue.size();
        }

        void clear()
        {
            m_queue.clear();
        }

    private:

        std::priority_queue<Type, std::vector<Type>, cmp> m_queue;

};

template<class Type,class QUEUE=std::queue<Type> >
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
        QUEUE m_queue;
};

#endif

