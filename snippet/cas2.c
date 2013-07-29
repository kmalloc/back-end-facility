#include <stdio.h>

#include "../sys/atomic_ops.h"

int main()
{
    __int128_t a = 1, b;
    DoublePointer dp;

    dp.val = 23;

    DoublePointer dp2 = atomic_read_double(&dp);

    printf("addr %x:\n", &dp);
    b = __sync_val_compare_and_swap(&a, 1, 2);
    return 0;
}
