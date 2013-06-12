#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

#include "worker.h"

#include <vector>


class Dispatcher;

class ThreadPool: public Worker 
{
    public:

        ThreadPool(int num = 0);
        ~ThreadPool();

        bool StartPooling();

        //calling this function will shutdown threadpool in an elegant way.
        //currently running task will not abort.
        //otherwise, on destruction,
        //destructor will shutdown all the threads using pthread_cancel,
        //which is totally out of control.
        void StopPooling();


        int SetWorkerNotify(Worker* worker);

    protected:

        int CalcDefaultThreadNum();
        using Worker::StartWorking;
        using Worker::GetWorkerId;

        Dispatcher* m_dispatcher;
};

#endif

