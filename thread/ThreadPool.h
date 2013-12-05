#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

#include "WorkerBodyBase.h"
#include "Worker.h"
#include "misc/NonCopyable.h"

class Dispatcher;

class ThreadPool: public WorkerManagerBase, public noncopyable
{
    public:

        ThreadPool(int num = 0);
        ~ThreadPool();

        bool PostTask(ITask*);
        bool IsRunning() const;
        int GetTaskNumber();
        bool StartPooling();

        //calling this function will shutdown threadpool in an elegant way.
        //currently running task will not abort.
        //otherwise, on destruction,
        //destructor will shutdown all the threads using pthread_cancel,
        //which is totally out of control.
        bool StopPooling();

        //shutdown threadpool immediately.
        //killing all workers.
        void ForceShutdown();

    protected:

        virtual int SetWorkerNotify(NotifyerBase* notifyer);

        virtual int CalcDefaultThreadNum() const;

        bool        running_;
        Worker*     worker_;
        Dispatcher* dispatcher_;
};

#endif

