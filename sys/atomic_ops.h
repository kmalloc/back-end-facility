#ifndef _ATMOMIC_H_
#define _ATMOMIC_H_

#include <stddef.h>


#define atomic_cas(ptr, oldVal, newVal)  __sync_bool_compare_and_swap(ptr, oldVal, newVal)

inline void atomic_add(volatile int * val, int gap)
{
    int oldval;
    int newval;

    do
    {
        oldval = *val;
        newval = oldval + gap;
    } while (!atomic_cas(val, oldval, newval));
}

inline void atomic_increment(volatile int * val)
{
    atomic_add(val, 1);
}

inline void atomic_decrement(volatile int * val)
{
    atomic_add(val, -1);
}

inline void atomic_increment(volatile size_t* val)
{
    size_t old_val;
    size_t new_val;

    do
    {
        old_val = *val;
        new_val = old_val + 1;
    } while (!atomic_cas(val, old_val, new_val));
}

inline void atomic_decrement(volatile size_t* val)
{
    size_t old_val;
    size_t new_val;

    do
    {
        old_val = *val;
        new_val = old_val - 1;
    } while (!atomic_cas(val, old_val, new_val));
}

#endif //_ATMOMIC_H_

