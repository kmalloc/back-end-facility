#ifndef _DEFS_H_
#define _DEFS_H_

#include <stdlib.h>

#define offset_of(TYPE, MEMBER) ((size_t)&((TYPE*)0)->MEMBER)

#define container_of(ptr, type, member) ({\
        const typeof( ((type *)0)->member)* __mptr =\
        (typeof( ((type *)0)->member)*)(ptr);\
        (type *) ( (char*)__mptr - offset_of(type, member) ); })


#define DEFAULT_WORKER_TASK_MSG_SIZE  (1024)

#endif

