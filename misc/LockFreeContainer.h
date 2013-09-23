#ifndef __LOCK_FREE_CONTAINER_H_
#define __LOCK_FREE_CONTAINER_H_

#include "AtomicOps.h"

#include <stdio.h>
#include <assert.h>
#include <stddef.h>
#include <pthread.h>

#include <linux/kernel.h>

#include "misc/NonCopyable.h"

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
class LockFreeStack: public noncopyable
{
    public:

        LockFreeStack(size_t sz)
            :m_top(0)
            ,m_maxSz(sz)
            ,m_mask(0)
            ,m_readMask(0xffff)
            ,m_writeMask(m_readMask << 16)
            ,m_maxConcurrentRead(0xff)
            ,m_maxConcurrntWrite(0xff)
        {
            m_arr = new Type[sz];
        } 

        ~LockFreeStack()
        {
            delete[] m_arr;
        }

        bool Push(const Type& val)
        {
            size_t old_mask, append;
            size_t old_top;
            bool   ret = true;

            append = ((0x01) << 16);

            while (1)
            {
                old_mask = (m_mask & (~m_readMask));

                if ((old_mask >> 16) >= (m_maxConcurrntWrite)) continue;

                if(atomic_cas(&m_mask, old_mask, old_mask + append)) break;
            } 

            //now the calling thread acquired 'write-lock'.

            //reserve a slot in stack.
            while (1)
            {
                old_top = m_top;

                if (old_top == m_maxSz) 
                {
                    ret = false;
                    break;
                }

                if (atomic_cas(&m_top, old_top, old_top + 1)) break;
            }

            if (ret) m_arr[old_top] = val;

            assert((m_mask & (~m_writeMask)) == 0);

            //release 'write-lock'
            while (1)
            {
                old_mask = m_mask;

                if (atomic_cas(&m_mask, old_mask, old_mask - append))
                    break;
            }

            return ret;
        }

        bool Pop(Type* val)
        {
            Type ret;
            bool   suc = true;
            size_t old_top;
            size_t old_mask, append = 0x01;

            while(1) 
            {
                old_mask = (m_mask & (~m_writeMask));

                if (old_mask >= m_maxConcurrentRead) continue;

                if (atomic_cas(&m_mask, old_mask, old_mask + append)) break;
            }  

            while (1)
            {
                old_top = m_top;

                if (old_top == 0)
                {
                    suc = false;
                    break;
                }

                if (atomic_cas(&m_top, old_top, old_top - 1)) break;
            }

            if (suc)
            {
                ret = m_arr[old_top - 1];
                m_arr[old_top - 1] = (Type)0xcdcd; //to detech corruption.
            }
            
            assert((m_mask & (~m_readMask)) == 0);

            while (1)
            {
                old_mask = m_mask;
                if (atomic_cas(&m_mask, old_mask, old_mask - append)) break;
            }

            if (val) *val = ret;

            return suc;
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
        const size_t m_maxSz;
        volatile size_t m_mask;
        const size_t m_readMask;
        const size_t m_writeMask;
        const size_t m_maxConcurrentRead;
        const size_t m_maxConcurrntWrite;
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
            
            //if calling thread dies or exits here, this queue will be in an abnormal state:
            //subsequent read or write to it will make the calling thread hang forever.
            //so, technically this lock free structure is not that true "lock free".
            while (!atomic_cas(&m_maxRead, old_write, (old_write + 1)%m_maxSz))
                sched_yield();

            return true;
        }


        bool Pop(Type* val)
        {
            Type ret;

            size_t old_read;

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

