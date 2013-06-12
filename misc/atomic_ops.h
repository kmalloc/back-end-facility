#ifndef _ATMOMIC_H_
#define _ATMOMIC_H_

#define cas(ptr, oldVal, newVal)  __sync_bool_compare_and_swap(ptr, oldVal, newVal)


inline void atomic_add(volatile long * val, long gap)
{
    long oldval;
    long newval;

    do
    {
        oldval = *val;
        newval = oldval + gap;
    } while (cas(val, oldval, newval);
}

inline void atomic_increment(volatile lone * val)
{
    atomic_add(val, 1);
}

inline void atomic_decrement(volatile lone * val)
{
    atomic_add(val, -1);
}

#endif

