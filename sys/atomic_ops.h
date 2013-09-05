#ifndef _ATMOMIC_H_
#define _ATMOMIC_H_

#define USING_CAS_TO_DO_INCREASE 1

#define atomic_cas(ptr, oldVal, newVal)  __sync_bool_compare_and_swap(ptr, oldVal, newVal)

#if USING_CAS_TO_DO_INCREASE

#include <stdlib.h>

inline int atomic_add(volatile int* val, int gap)
{
    int oldval;
    int newval;

    do
    {
        oldval = *val;
        newval = oldval + gap;
    } while (!atomic_cas(val, oldval, newval));

    return oldval;
}

inline int atomic_increment(volatile int* val)
{
    return atomic_add(val, 1);
}

inline int atomic_decrement(volatile int* val)
{
    return atomic_add(val, -1);
}

inline size_t atomic_increment(volatile size_t* val)
{
    size_t old_val;
    size_t new_val;

    do
    {
        old_val = *val;
        new_val = old_val + 1;
    } while (!atomic_cas(val, old_val, new_val));

    return old_val;
}

inline size_t atomic_decrement(volatile size_t* val)
{
    size_t old_val;
    size_t new_val;

    do
    {
        old_val = *val;
        new_val = old_val - 1;
    } while (!atomic_cas(val, old_val, new_val));

    return old_val;
}

#else

#define atomic_add(ptr, margin)  __sync_fetch_and_add(ptr, margin)
#define atomic_sub(ptr, margin)  __sync_fetch_and_sub(ptr, margin)
#define atomic_increment(ptr)    __sync_fetch_and_add(ptr, 1)
#define atomic_decrement(ptr)    __sync_fetch_and_sub(ptr, 1)

#endif // USING_CAS_TO_DO_INCREASE


/////////////////////////setting up cas /////////////////////////////

#include <stdint.h>

#if defined(__x86_64__)

typedef __int128_t atomic_longlong;

#else

typedef int64_t atomic_longlong;

#endif // x86-64



#define atomic_cas2(ptr, oldVal, newVal)   __sync_bool_compare_and_swap((volatile atomic_longlong*)ptr, oldVal, newVal)
#define atomic_read(ptr)  __sync_fetch_and_add(ptr, 0)
#define atomic_read_double(ptr)  __sync_fetch_and_add((volatile atomic_longlong*)ptr, 0)

union DoublePointer 
{
    void* vals[2];
    volatile atomic_longlong val;

    DoublePointer() { val = 0; }
    DoublePointer(const atomic_longlong& value) { val = value; }

    operator atomic_longlong() { return val; }
} __attribute__((aligned(sizeof(atomic_longlong))));


inline void InitDoublePointer(volatile DoublePointer& dp)
{
    dp.val = 0;
}

inline bool IsDoublePointerNull(const DoublePointer& dp)
{
    return dp.val == 0;
}

inline bool IsDoublePointerEqual(const DoublePointer& dp1, const DoublePointer& dp2)
{
    return dp1.val == dp2.val;
}

inline void SetDoublePointer(volatile DoublePointer& dp, void* lo, void* hi)
{
    dp.vals[0] = lo;
    dp.vals[1] = hi;
}

inline size_t AlignTo(size_t size, size_t align)
{
    return size%align?size - size%align + align:size;
}

#endif //_ATMOMIC_H_

