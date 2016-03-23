#ifndef __LOCK_FREE_CONTAINER_H_
#define __LOCK_FREE_CONTAINER_H_

#include <stdio.h>
#include <assert.h>
#include <stddef.h>
#include <pthread.h>

#include <linux/kernel.h>

#include "sys/AtomicOps.h"
#include "misc/NonCopyable.h"

//note:
//(a)
//to ensure efficiency of manipulating these structure,
//better use them to store pointer only
//(b)
//if a thread calling pop/push die before the function finishs.
//later calling to pop/push will be blocking forever

//memory model is conceptually not an concern here according to the gnu document:
//gcc.gnu.org/onlinedocs/gcc-4.1.2/gcc/atomic-builtins.html:
//atomic operations are considerd "full barrier".
//so we don't even need to set memory barrier explicitly.

//One thing to note: the following code is designed only for x86/x64.

template<class Type>
class LockFreeStack: public noncopyable
{
    public:
        explicit LockFreeStack(size_t sz)
            : top_(0), maxSz_(sz)
            , mask_(0), readMask_(0xffff)
            , writeMask_(readMask_ << 16)
            , maxConcurrentRead_(0xff)
            , maxConcurrntWrite_(0xff)
        {
            arr_ = new Type[sz];
        }

        ~LockFreeStack()
        {
            delete[] arr_;
        }

        bool Push(const Type& val)
        {
            size_t old_mask, append;
            size_t old_top;
            bool   ret = true;

            append = ((0x01) << 16);

            while (1)
            {
                old_mask = (atomic_read(&mask_) & (~readMask_));

                if ((old_mask >> 16) >= (maxConcurrntWrite_)) continue;

                if(atomic_cas(&mask_, old_mask, old_mask + append)) break;
            }

            //now the calling thread acquired 'write-lock'.

            //reserve a slot in stack.
            while (1)
            {
                old_top = atomic_read(&top_);

                if (old_top == maxSz_)
                {
                    ret = false;
                    break;
                }

                if (atomic_cas(&top_, old_top, old_top + 1)) break;
            }

            if (ret) arr_[old_top] = val;

            assert((atomic_read(&mask_) & (~writeMask_)) == 0);

            //release 'write-lock'
            while (1)
            {
                old_mask = mask_;

                if (atomic_cas(&mask_, old_mask, old_mask - append))
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
                old_mask = (atomic_read(&mask_) & (~writeMask_));

                if (old_mask >= maxConcurrentRead_) continue;

                if (atomic_cas(&mask_, old_mask, old_mask + append)) break;
            }

            while (1)
            {
                old_top = atomic_read(&top_);

                if (old_top == 0)
                {
                    suc = false;
                    break;
                }

                if (atomic_cas(&top_, old_top, old_top - 1)) break;
            }

            if (suc)
            {
                ret = arr_[old_top - 1];
                arr_[old_top - 1] = (Type)0xcdcd; //to detech corruption.
            }

            assert((atomic_read(&mask_) & (~readMask_)) == 0);

            while (1)
            {
                old_mask = mask_;
                if (atomic_cas(&mask_, old_mask, old_mask - append)) break;
            }

            if (val) *val = ret;

            return suc;
        }

        bool IsEmpty() const
        {
            return atomic_read(&top_);
        }

        size_t Size() const
        {
            return atomic_read(&top_);
        }

    private:

        Type*  arr_;
        volatile size_t top_;
        const size_t maxSz_;
        volatile size_t mask_;
        const size_t readMask_;
        const size_t writeMask_;
        const size_t maxConcurrentRead_;
        const size_t maxConcurrntWrite_;
};

template<class Type>
class LockFreeQueue
{
    public:
        explicit LockFreeQueue(int sz)
            : maxSz_(sz), read_(0)
            , write_(0), maxRead_(0)
            , size_(0)
        {
            arr_ = new Type[sz];
        }

        ~LockFreeQueue()
        {
            delete[] arr_;
        }

        bool Push(Type val)
        {
            size_t old_write, old_read;

            do
            {
                old_write = atomic_read(&write_);
                old_read  = atomic_read(&read_);

                if ((old_write + 1)%maxSz_ == old_read) return false;

            } while(!atomic_cas(&write_, old_write, (old_write + 1)%maxSz_));

            arr_[old_write] = val;

            //if calling thread dies or exits here, this queue will be in an abnormal state:
            //subsequent read or write to it will make the calling thread hang forever.
            //so, technically this lock free structure is not that true "lock free".
            while (!atomic_cas(&maxRead_, old_write, (old_write + 1)%maxSz_))
                sched_yield();

            return true;
        }

        bool Pop(Type* val)
        {
            Type ret;
            size_t old_read;

            do
            {
                old_read = atomic_read(&read_);

                if (old_read == atomic_read(&maxRead_)) return false;

                ret = arr_[old_read];

            } while (!atomic_cas(&read_, old_read, (old_read + 1)%maxSz_));

            if (val) *val = ret;

            return true;
        }

        bool IsEmpty() const
        {
            return atomic_read(&read_) == atomic_read(&write_);
        }

        int Size() const
        {
            return (atomic_read(&write_) + maxSz_ - atomic_read(&read_))%maxSz_;
        }

    private:
        Type*  arr_;
        const size_t maxSz_;
        volatile size_t read_; // read position
        volatile size_t write_; // write position
        volatile size_t maxRead_;
        volatile size_t size_;
};


#endif

