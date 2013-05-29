#ifndef _WORKER_THREAD_H_
#define _WORKER_THREAD_H_

#include "Thread.h"


class Worker:public Thread
{
    public:
        Worker();
        ~Worker();
};

#endif

