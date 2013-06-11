#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

#include "worker.h"

#include <vector>


class ThreadPool: public Worker 
{
    public:

        ThreadPool(int num = 0);
        ~ThreadPool();

    protected:

        int CalcDefaultThreadNum();
};

#endif

