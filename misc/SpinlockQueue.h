#ifndef _SPIN_LOCK_QUEUE_H_
#define _SPIN_LOCK_QUEUE_H_

#include "NonCopyable.h"
#include "sys/Defs.h"

#include <vector>
#include <queue>
#include <pthread.h>
#include <semaphore.h>

template<class Type, class cmp = std::less<Type> >
class PriorityQueue: public noncopyable
{
    public:
        PriorityQueue() {}
        ~PriorityQueue(){}

        const Type& front()
        {
            return queue_.top();
        }

        void pop()
        {
            queue_.pop();
        }

        void push(const Type& x)
        {
            queue_.push(x);
        }

        bool empty() const
        {
            return queue_.empty();
        }

        size_t size() const
        {
            return queue_.size();
        }

        void clear()
        {
            queue_ = std::priority_queue<Type, std::vector<Type>, cmp>();
        }

    private:

        std::priority_queue<Type, std::vector<Type>, cmp> queue_;

};

template<class Type>
class DequeQueue
{
    public:
        DequeQueue() {}
        ~DequeQueue() {}

        const Type& front()
        {
            return queue_.front();
        }

        void pop()
        {
            queue_.pop_front();
        }

        void push(const Type& x)
        {
            queue_.push_back(x);
        }

        void push_front(const Type& x)
        {
            queue_.push_front(x);
        }

        bool empty() const
        {
            return queue_.empty();
        }

        size_t size() const
        {
            return queue_.size();
        }

        void clear()
        {
            queue_.clear();
        }

    protected:

        std::deque<Type> queue_;
};

template<class Type,class QUEUE=std::queue<Type> >
class SpinlockQueue
{
    public:

        explicit SpinlockQueue(int Maxsize = DEFAULT_WORKER_TASK_MSG_SIZE)
            :maxSz_(Maxsize)
        {
            pthread_spin_init(&lock_,0);
        }

        ~SpinlockQueue()
        {
        }

        bool PopFront(Type* ret = NULL)
        {
            Type val;

            pthread_spin_lock(&lock_);

            if (queue_.empty())
            {
                pthread_spin_unlock(&lock_);
                return false;
            }

            val = queue_.front();
            queue_.pop();
            pthread_spin_unlock(&lock_);

            if (ret) *ret = val;

            return true;
        }

        bool PushBack(const Type& val)
        {
            bool full = true;

            pthread_spin_lock(&lock_);
            full = queue_.size() >= maxSz_;

            if (!full)
            {
                queue_.push(val);
            }

            pthread_spin_unlock(&lock_);

            return !full;
        }

        void Clear()
        {
            pthread_spin_lock(&lock_);
            queue_.clear();
            pthread_spin_unlock(&lock_);
        }

        int  Size() const
        {
            int count = -1;

            pthread_spin_lock(&lock_);
            count = queue_.size();
            pthread_spin_unlock(&lock_);

            return count;
        }

        bool IsEmpty() const
        {
            bool ret;

            pthread_spin_lock(&lock_);
            ret = queue_.empty();
            pthread_spin_unlock(&lock_);

            return ret;
        }

    protected:

        const size_t maxSz_;
        mutable pthread_spinlock_t lock_;
        QUEUE queue_;
};


template<class Type>
class SpinlockWeakPriorityQueue: public SpinlockQueue<Type,DequeQueue<Type> >
{
    public:

        explicit SpinlockWeakPriorityQueue(int maxSz = DEFAULT_WORKER_TASK_MSG_SIZE): SpinlockQueue<Type,DequeQueue<Type> >(maxSz){}
        ~SpinlockWeakPriorityQueue() {}


        bool PushFront(const Type& val)
        {
            bool full = true;

            pthread_spin_lock(&this->lock_);
            full = this->queue_.size() >= this->maxSz_;

            if (!full)
            {
                this->queue_.push_front(val);
            }

            pthread_spin_unlock(&this->lock_);

            return !full;
        }

};

#endif

