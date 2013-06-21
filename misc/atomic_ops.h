#ifndef _ATMOMIC_H_
#define _ATMOMIC_H_

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

#endif //_ATMOMIC_H_

