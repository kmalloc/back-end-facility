#ifndef __LOCK_FREE_CONTAINER_H_
#define __LOCK_FREE_CONTAINER_H_

#include "atomic_ops.h"

//note:
//to avoid false sharing issue.
//make sure entry size is aligned to cache line.

template<class Type>
class LockFreeStack
{
};



template<class Type>
class LockFreeQueue
{
};


#endif

