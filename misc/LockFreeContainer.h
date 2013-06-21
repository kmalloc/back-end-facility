#ifndef __LOCK_FREE_CONTAINER_H_
#define __LOCK_FREE_CONTAINER_H_

#include "atomic_ops.h"

//note:
//to avoid false sharing issue.
//make sure entry size is aligned to cache line.

template<class Type>
class LockFreeStack
{
    public:

        LockFreeStack(size_t sz)
            :m_top(0), m_maxSz(sz)
        {
            m_arr = new Type[sz];
        }

        ~LockFreeStack()
        {
            delete m_arr;
        }

        void SetNull(Type val)
        {
            m_null = val;
        }

        bool Push(const Type& val)
        {
            size_t old;

            do
            {
                old = m_top;

                if (old == m_maxSz) return false;

            } while(!atomic_cas(&m_top, old, old + 1));

            //now slot is reserved
            //but it is not safe to set the entry yet.
            //the follow code is not correct.
            //need to find a way to guarantee that:
            //no pop operation will take place until we finish setting the value.
            //TODO
            m_arr[old] = val;

            return true;
        }

        Type Pop()
        {
            Type ret;
            size_t old;

            do
            {
                old = m_top;
                
                if (old == 0) return m_null;

                ret = m_arr[old - 1];

            } while(!atomic_cas(&m_top, old, old - 1));


            return ret;
        }

        bool IsEmpty() const
        {
            return m_top == 0;
        }

        size_t Size() const
        {
            return m_top;
        }


    private:

        size_t m_maxSz;
        Type   m_null;
        size_t m_top;
        size_t m_read;
        Type*  m_arr;
};



template<class Type>
class LockFreeQueue
{
};


#endif

