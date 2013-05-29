#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

#include "Thread.h"


class ThreadPool:public Thread
{
    public:

        ThreadPool(int num = 0);
        ~ThreadPool();

    private:

        friend class Dispatcher;

        int m_num;
        ITask* m_dispatch;

        std::vector<Thread*> m_workers;
};

#endif

