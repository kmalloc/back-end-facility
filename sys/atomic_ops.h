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

typedef __int128_t  __int128;

/*
struct DoublePointe2
{
    void* volatile lo;
    void* volatile hi;

    DoublePointer() { lo = (void*)0; hi = (void*)0; }
    DoublePointer(const DoublePointer& val) { lo = val.lo; hi = val.hi; }
    DoublePointer(__int128_t val) { DoublePointer* vp = (DoublePointer*)&val; lo =  vp->lo; hi = vp->hi; }
    DoublePointer(volatile const DoublePointer* val) { lo = val->lo; hi = val->hi; }
    operator __int128_t() const { return *(__int128_t*)this; }
    operator __int128_t*() { return (__int128_t*)this; }
    DoublePointer& operator=(const DoublePointer& val) { lo = val.lo; hi = val.hi; return *this; }
} __attribute__((aligned(16)));
*/

union DoublePointer 
{
    void* vals[2];
    volatile __int128_t val;

    DoublePointer() { val = 0; }
    DoublePointer(const __int128_t& value) { val = value; }
};

inline bool atomic_cas_16(volatile DoublePointer* src, const DoublePointer& oldVal, const DoublePointer& newVal)
{
    // from intel ia32-64 developer manual, vol.2a 3-149,
    // instruction "cmpxchg16b" takes the follow format:
    // cmpxchg16b m128.
    // meaning:
    // compare rdx:rax with m128, if equal , set zf, and 
    // load rcx:rbx into m128,
    // else, clear zf and, load m128 into rdx:rax
    bool result;

    __asm__ __volatile__
        (
         "lock cmpxchg16b %1;\n"
         "setz %0;\n"
         : "=A"(result), "+m"(*src)
         : "d"(oldVal.vals[1]), "a"(oldVal.vals[0]), "c"(newVal.vals[1]), "b"(newVal.vals[0])
         //: "cc","memory"
        );

    return result;
}

#define atomic_cas2(ptr, oldVal, newVal)   __sync_bool_compare_and_swap((volatile __int128_t*)ptr, oldVal, newVal)

inline DoublePointer atomic_read_double(volatile DoublePointer* val)
{
    __int128_t old = val->val;

    do 
    {
        old = val->val;
        if (atomic_cas2(val, old, old))
            break;

    } while (1);

    return old;
}


#else

struct DoublePointer
{
    void* volatile lo;
    void* volatile hi;
    DoublePointer() { lo = (void*)0; hi = (void*)0; }
    DoublePointer(const DoublePointer& dp) { lo = dp.lo; hi = dp.hi; }
    DoublePointer(uint64_t val) { lo = (void*)(val & (uint64_t)0x00000000ffffffff); hi = (void*)(val >> (8 * sizeof(void*))); }
    operator uint64_t() const{ return *(uint64_t*)this; }
} __attribute__((aligned(8)));

#define atomic_cas2(ptr, oldVal, newVal)  atomic_cas((uint64_t*)ptr, oldVal, newVal)
#define atomic_read_double(ptr)  __sync_fetch_and_add((uint64_t*)ptr, 0)

#endif // x86-64

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

#endif //_ATMOMIC_H_

