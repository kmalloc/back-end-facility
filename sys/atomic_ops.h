#ifndef _ATMOMIC_H_
#define _ATMOMIC_H_

#define atomic_cas(ptr, oldVal, newVal)  __sync_bool_compare_and_swap(ptr, oldVal, newVal)

#define atomic_add(ptr, margin)  __sync_fetch_and_add(ptr, margin)
#define atomic_sub(ptr, margin)  __sync_fetch_and_sub(ptr, margin)
#define atomic_increment(ptr)    __sync_fetch_and_add(ptr, 1)
#define atomic_decrement(ptr)    __sync_fetch_and_sub(ptr, 1)


/////////////////////////setting up cas /////////////////////////////

#include <stdint.h>

#if defined(__x86_64__)

struct DoublePointer
{
    void* volatile lo;
    void* volatile hi;
    DoublePointer() { lo = (void*)0; hi = (void*)0; }
} __attribute__((aligned(16)));

typedef DoublePointer atomic_uint128;

inline bool atomic_cas_16(volatile DoublePointer* src, DoublePointer oldVal, DoublePointer newVal)
{
    // intel ia32-64 developer manual, vol.2a 3-149.
    // cmpxchg16b m128.
    // compare rdx:rax with m128, if equal , set zf, and 
    // load rcx:rbx into m128,
    // else, clear zf and, load m128 into rdx:rax
    bool result;
    __asm__ __volatile__
        (
         "lock cmpxchg16b %1\n\t"
         "setz %0"
         : "=q"(result), "+m"(*src), "+d"(oldVal.hi), "+a"(oldVal.lo)
         : "c"(newVal.hi), "b"(newVal.lo)
         : "cc","memory"
        );
    return result;
}

#define atomic_cas2(ptr, oldVal, newVal)   atomic_cas_16(ptr, oldVal, newVal)

#else

struct DoublePointer
{
    void* volatile lo;
    void* volatile hi;
    DoublePointer() { lo = (void*)0; hi = (void*)0; }
    DoublePointer(const DoublePointer& dp) { lo = dp.lo; hi = dp.hi; }
    DoublePointer(uint64_t val) { lo = (void*)(val & 0x00000000ffffffff); hi = (void*)((val & 0xffffffff00000000) >> (8 * 4)); }
    operator uint64_t() { return *(uint64_t*)this; }
} __attribute__((aligned(8)));

#define atomic_cas2(ptr, oldVal, newVal)  atomic_cas((uint64_t*)ptr, oldVal, newVal)
#define atomic_read_double(ptr)  __sync_fetch_and_add((uint64_t*)ptr, 0)

#endif // x86-64

inline void InitDoublePointer(volatile DoublePointer& dp)
{
    dp.lo = (void*)0;
    dp.hi = (void*)0;
}

inline bool IsDoublePointerNull(const DoublePointer& dp)
{
    return (dp.lo == (void*)0 && dp.hi == (void*)0);
}

inline bool IsDoublePointerEqual(const DoublePointer& dp1, const DoublePointer& dp2)
{
    return ((dp1.lo == dp2.lo) && (dp1.hi == dp2.hi));
}

inline void SetDoublePointer(volatile DoublePointer& dp, void* lo, void* hi)
{
    dp.lo = lo;
    dp.hi = hi;
}

#endif //_ATMOMIC_H_

