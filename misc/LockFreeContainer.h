#ifndef __LOCK_FREE_CONTAINER_H_
#define __LOCK_FREE_CONTAINER_H_

#include "atomic_ops.h"

#include <assert.h>
#include <stddef.h>

//note:
//(a)
//to ensure efficiency of manipulating these structure,
//better use them to store pointer only
//(b)
//if a thread calling pop/push die before the function finishs .
//later calling to pop/push will be blocking forever

//memory model is conceptually not an concern here according to the gnu document:
//gcc.gnu.org/onlinedocs/gcc-4.1.2/gcc/atomic-builtins.html:
//atomic operations are considerd "full barrier".
//so we don't even need to set memory barrier explicitly.
//One thing to note: the following code is targetting for x86/x64 only.
//I don't even test them on other processor architecture.


template<class Type>
class LockFreeStack
{
    public:

        LockFreeStack(size_t sz)
            :m_top(0), m_popIndex(0), m_maxSz(sz)
        {
            m_arr = new Type[sz];
        }

        ~LockFreeStack()
        {
            delete[] m_arr;
        }

        bool Push(const Type& val)
        {
            size_t old_top, old_pop;

            while (1)
            {
                old_top = m_top;
                old_pop = m_popIndex;

                if (old_top == m_maxSz) return false;

                if (old_pop != old_top) continue;

                if (atomic_cas(&m_top, old_pop, old_pop + 1)) break; //aba issue still there, to be fixed.

                //solution: 
                //put a constraint that size of the stack is no larger then 0xffff for 32 bit version.
                //then use upper two bytes of m_top to set flag.
                //set up flag to indicate that writing/reading is being process.
                //
                //atomic_cas(&m_top, old_pop&0x0001ffff, 0xffff0000|(old_pop + 1))
                //
             
            }  

            m_arr[m_top - 1] = val;

            atomic_increment(&m_popIndex);

            return true;
        }

        bool Pop(Type* val)
        {
            Type ret;
            size_t old_top;
            size_t old_pop;

            while (1) 
            {
                old_top = m_top;
                old_pop = m_popIndex;
                
                if (old_top == 0) return false;

                if (old_top != old_pop) continue; 

                if (atomic_cas(&m_top, old_pop, old_pop - 1)) break;
            } 

            ret = m_arr[old_pop - 1];
            m_arr[old_pop - 1] = (Type)0xcdcd;//for test, catch aba issue

            atomic_decrement(&m_popIndex);

            if (val) *val = ret;

            return true;
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

        Type*  m_arr;
        volatile size_t m_top;
        volatile size_t m_popIndex;
        const size_t m_maxSz;
};



template<class Type>
class LockFreeQueue
{
    public:

        LockFreeQueue(int sz)
            :m_maxSz(sz)
            ,m_read(0)
            ,m_write(0)
            ,m_maxRead(0)
            ,m_size(0)
        {
            m_arr = new Type[sz];
        }

        ~LockFreeQueue()
        {
            delete[] m_arr;
        }

        bool Push(Type val)
        {
            size_t old_write, old_read;

            do
            {
                old_write = m_write;
                old_read  = m_read;

                if ((old_write + 1)%m_maxSz == old_read) return false;

            } while(!atomic_cas(&m_write, old_write, (old_write + 1)%m_maxSz));

            m_arr[old_write] = val;
            
            while (!atomic_cas(&m_maxRead, old_write, (old_write + 1)%m_maxSz));

            return true;
        }


        bool Pop(Type* val)
        {
            Type ret;

            size_t old_read;
            size_t old_write;

            do
            {
                old_read = m_read;

                if (old_read == m_maxRead) return false;

                ret = m_arr[old_read];

            } while (!atomic_cas(&m_read, old_read, (old_read + 1)%m_maxSz));

            if (val) *val = ret;

            return true;
        }

        bool IsEmpty() const
        {
            return m_read == m_write;
        }

        int Size() const
        {
            return (m_write + m_maxSz - m_read)%m_maxSz;
        }

    private:

        Type*  m_arr;
        const size_t m_maxSz;
        volatile size_t m_read; // read position
        volatile size_t m_write; // write position
        volatile size_t m_maxRead;
        volatile size_t m_size;
};


#endif

