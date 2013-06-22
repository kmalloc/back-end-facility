#ifndef __LOCK_FREE_CONTAINER_H_
#define __LOCK_FREE_CONTAINER_H_

#include "atomic_ops.h"

//note:
//(a)
//to ensure efficiency of these structure,
//better use them to store pointer only
//(b)
//if a thread calling pop/push die before the function finishs .
//later calling to pop/push will be blocking forever

template<class Type>
class LockFreeStack
{
    public:

        LockFreeStack(size_t sz)
            :m_top(0), m_popIndex(-1), m_maxSz(sz)
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
            size_t old_top, old_pop;

            do
            {
                old_top = m_top;
                old_pop = m_popIndex;

                if (old_top == m_maxSz) return false;

                if (old_top == old_pop)
                {
                    atomic_cas(&m_popIndex, old_pop, old_pop - 1);
                    continue;
                }

            } while(old_top - 1 != old_pop || !atomic_cas(&m_top, old, old + 1));

            m_arr[old] = val;

            atomic_cas(&m_popIndex, old_pop, old_pop + 1);

            return true;
        }

        Type Pop()
        {
            Type ret;
            size_t old_top;
            size_t old_pop;

            do
            {
                old_top = m_top;
                old_pop = m_popIndex;
                
                if (old_top == 0) return m_null;

                if (old_top == old_pop)
                {
                    atomic_cas(&m_popIndex, old_pop, old_pop - 1);
                    continue;
                }

                ret = m_arr[old_top - 1];

            } while(old_top - 1 != old_pop || !atomic_cas(&m_top, old_top, old_top - 1));

            atomic_cas(&m_popIndex, old_pop, old_pop - 1);

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

        Type   m_null;
        Type*  m_arr;
        size_t m_top;
        size_t m_popIndex;
        size_t m_maxSz;
};



template<class Type>
class LockFreeQueue
{
};


#endif

