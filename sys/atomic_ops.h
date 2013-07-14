#ifndef _ATMOMIC_H_
#define _ATMOMIC_H_

#define atomic_cas(ptr, oldVal, newVal)  __sync_bool_compare_and_swap(ptr, oldVal, newVal)

#define atomic_add(ptr, margin)  __sync_fetch_and_add(ptr, margin)
#define atomic_sub(ptr, margin)  __sync_fetch_and_sub(ptr, margin)
#define atomic_increment(ptr)    __sync_fetch_and_add(ptr, 1)
#define atomic_decrement(ptr)    __sync_fetch_and_sub(ptr, 1)

#if defined(__x86_64__)
#include <stdint.h>

struct atomic_uint128
{
    uint64_t lo;
    uint64_t hi;
} __attribute__((aligned(16)));

inline bool atomic_cas_16(volatile atomic_uint128* src, atomic_uint128 oldVal, atomic_uint128 newVal)
{
    // intel ia32-64 developer manual, vol.2a 3-149.
    // cmpxchg16b m128.
    // compare rdx:rax with m128, if equal , set zf, and 
    // load rcx:rbx into m128,
    // else, clear zf and, load m128 into rdx:rax
    bool result;
    __asm__ __volatile__
        (
         "lock cmpxchg16b oword ptr %1\n\t"
         "setz %0"
         : "=q" (result)
         , "+m" (*src)
         , "+d" (oldVal.hi)
         , "+a" (oldVal.lo)
         : "c"  (newVal.hi)
         , "b"  (newVal.lo)
         : "cc"
        );
    return result;
}

#endif // x86-64

#endif //_ATMOMIC_H_

